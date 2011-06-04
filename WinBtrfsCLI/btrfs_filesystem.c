/* btrfs_filesystem.c
 * nitty gritty filesystem functionality
 *
 * WinBtrfs
 *
 * Copyright (c) 2011 Justin Gottula
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <stdio.h>
#include <assert.h>
#include <Windows.h>
#include "structures.h"
#include "endian.h"
#include "crc32c.h"

extern WCHAR devicePath[MAX_PATH];

HANDLE hRead = INVALID_HANDLE_VALUE, hReadMutex = INVALID_HANDLE_VALUE;
Superblock super;
BtrfsDevItem *devices = NULL;
int numDevices = -1;
Chunk *chunks = NULL;
int numChunks = -1;

DWORD init()
{
	hRead = CreateFile(devicePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	if (hRead == INVALID_HANDLE_VALUE)
		return GetLastError();

	hReadMutex = CreateMutex(NULL, FALSE, NULL);

	if (hReadMutex == INVALID_HANDLE_VALUE)
		return GetLastError();

	return 0;
}

void cleanUp()
{
	CloseHandle(hRead);
}

DWORD readBlock(LONGLONG physAddr, DWORD len, LPVOID dest)
{
	LARGE_INTEGER li;
	DWORD bytesRead;

	/* these had better be set up already */
	assert(hRead != INVALID_HANDLE_VALUE);
	assert(hReadMutex != INVALID_HANDLE_VALUE);

	li.QuadPart = physAddr;

	/* using SetFilePointerEx and then ReadFile is clearly not threadsafe, so for now
		I'm using a mutex here; in the future, this should be fully threaded
		(perhaps a separate read handle per thread? this would require a CreateFile on each
		call—or perhaps declare a static local that would retain value per thread?) */
	if (WaitForSingleObject(hReadMutex, 10000) != WAIT_OBJECT_0)
		goto error;

	if (SetFilePointerEx(hRead, li, NULL, 0) == 0)
		goto error;

	if (ReadFile(hRead, dest, len, &bytesRead, NULL) == 0 || bytesRead != len)
		goto error;

	ReleaseMutex(hReadMutex);
	return 0;

error:
	ReleaseMutex(hReadMutex);
	return GetLastError();
}

DWORD readLogicalBlock(LONGLONG logiAddr, DWORD len, LPVOID dest)
{
	int i, chunk = -1;
	unsigned __int64 physAddr;

	/* cannot possibly succeed unless chunks have been loaded */
	assert(numChunks > 0 && chunks != NULL);
	
	for (i = 0; i < numChunks; i++)
	{
		/* find the first chunk mapping that fits the block we want */
		if (logiAddr >= chunks[i].logiOffset && logiAddr + len <= chunks[i].logiOffset + chunks[i].chunkItem.chunkSize)
		{
			chunk = i;
			break;
		}
	}

	/* failure is NOT an option */
	assert(chunk != -1);

	/* arbitrarily using the first stripe */
	/* currently ASSUMING that everything is on the first device */
	physAddr = (logiAddr - chunks[chunk].logiOffset) + chunks[chunk].stripes[0].offset;

	return readBlock(physAddr, len, dest);
}

DWORD readPrimarySB()
{
	return readBlock(0x10000, sizeof(Superblock), &super);
}

int validateSB(Superblock *s)
{
	const char magic[8] =
	{
		'_', 'B', 'H', 'R', 'f', 'S', '_', 'M'
	};

	/* if no superblock is provided, use the primary one */
	if (s == NULL)
		s = &super;

	/* magic check */
	if (memcmp(magic, s->magic, 8) != 0)
		return 1;

	/* checksum */
	if (crc32c(0, (const unsigned char *)s + 0x20, 0xfe0) != endian32(s->crc32c))
		return 2;

	return 0;
}

int findSecondarySBs()
{
	Superblock s2, s3, s4, *sBest = &super;
	int best = 1;

	/* read each superblock (if present) and validate */

	if (readBlock(0x4000000, sizeof(Superblock), &s2) == 0 && validateSB(&s2) == 0 &&
		s2.generation > sBest->generation)
		best = 2, sBest = &s2;

	if (readBlock(0x4000000000, sizeof(Superblock), &s3) == 0 && validateSB(&s3) == 0 &&
		s3.generation > sBest->generation)
		best = 3, sBest = &s3;

	if (readBlock(0x4000000000000, sizeof(Superblock), &s4) == 0 && validateSB(&s4) == 0 &&
		s4.generation > sBest->generation)
		best = 4, sBest = &s4;

	/* replace the superblock in memory with the most up-to-date on-disk copy */

	if (best != 1)
		memcpy(&super, sBest, sizeof(Superblock));

	return best;
}

void getChunkItems()
{
	unsigned char *sbPtr = super.chunkData, *sbMax = super.chunkData + super.n;
	BtrfsDiskKey *key;
	Chunk *chunk;
	int i;

	/* this function only needs to be run ONCE */
	assert(chunks == NULL && numChunks == -1);

	while (sbPtr < sbMax)
	{
		if (sbPtr == super.chunkData)
		{
			numChunks = 1;
			chunks = (Chunk *)malloc(sizeof(Chunk));
		}
		else
		{
			numChunks++;
			chunks = (Chunk *)realloc(chunks, sizeof(Chunk) * numChunks);
		}

		chunk = &(chunks[numChunks - 1]);
		
		key = (BtrfsDiskKey *)((unsigned char *)sbPtr);
		sbPtr += sizeof(BtrfsDiskKey);

		/* CHUNK_ITEMs should ALWAYS fit these constraints */
		assert(key->objectID == 0x100);
		assert(key->type == 0xe4);
		
		chunk->logiOffset = key->offset;
		
		chunk->chunkItem = *((BtrfsChunkItem *)((unsigned char *)sbPtr));
		sbPtr += sizeof(BtrfsChunkItem);

		/* this should always be 2 */
		assert(chunk->chunkItem.rootObjIDref == 2);

		chunk->stripes = (BtrfsChunkItemStripe *)malloc(sizeof(BtrfsChunkItemStripe) * chunk->chunkItem.numStripes);

		for (i = 0; i < chunk->chunkItem.numStripes; i++)
		{
			chunk->stripes[i] = *((BtrfsChunkItemStripe *)sbPtr);
			sbPtr += sizeof(BtrfsChunkItemStripe);
		}
	}
}

void getChunkTree()
{
	unsigned __int64 chunkTreeLogiAddr = super.chunkTreeLAddr;
	int i, j, chunk = -1, stripe = -1;
	unsigned char *chunkTreeBlock, *chunkTreePtr;
	BtrfsHeader chunkTreeHeader;
	BtrfsItem *chunkTreeItem;

	/* find a superblock chunk to bootstrap ourselves to the chunk tree */
	for (i = 0; i < numChunks; i++)
	{
		if (chunkTreeLogiAddr >= chunks[i].logiOffset && chunkTreeLogiAddr < chunks[i].logiOffset + chunks[i].chunkItem.chunkSize)
		{
			for (j = 0; j < chunks[i].chunkItem.numStripes; j++)
			{
				if (chunks[i].stripes[j].devID == super.devItem.devID)
				{
					stripe = j;
					break;
				}
			}

			if (stripe != -1)
			{
				chunk = i;
				break;
			}
		}
	}

	/* we MUST have at least one mapping so we can load the chunk containing the chunk tree! */
	assert(chunk != -1 && stripe != -1);

	chunkTreeBlock = (unsigned char *)malloc(chunks[chunk].chunkItem.minIOSize);
	assert(readBlock(chunks[chunk].stripes[stripe].offset + (chunkTreeLogiAddr - chunks[chunk].logiOffset),
		chunks[chunk].chunkItem.minIOSize, chunkTreeBlock) == 0);

	chunkTreePtr = chunkTreeBlock;

	chunkTreeHeader = *((BtrfsHeader *)chunkTreePtr);
	chunkTreePtr += sizeof(BtrfsHeader);

	/* the crc32c had better check out */
	assert(crc32c(0, chunkTreeBlock + 0x20, chunks[chunk].chunkItem.minIOSize - 0x20) ==
		endian32(chunkTreeHeader.crc32c));

	/* this should definitely be a leaf node */
	assert(chunkTreeHeader.level == 0);

	/* clear up the chunks array in preparation for this larger collection of chunks;
		note that we don't have to free the main array, we just realloc it instead*/
	for (i = 0; i < numChunks; i++)
		free(chunks[i].stripes);

	/* set up the chunks array using the maximum possible number of items as the array size */
	chunks = (Chunk *)realloc(chunks, sizeof(Chunk) * chunkTreeHeader.nrItems);
	numChunks = 0;

	/* also set up the devices array, again using the max possible number */
	devices = (BtrfsDevItem *)malloc(sizeof(BtrfsDevItem) * chunkTreeHeader.nrItems);
	numDevices = 0;

	for (i = 0; i < chunkTreeHeader.nrItems; i++)
	{
		chunkTreeItem = (BtrfsItem *)chunkTreePtr;

		if (chunkTreeItem->key.type == 0xd8) // DEV_ITEM
		{
			assert(chunkTreeItem->size == sizeof(BtrfsDevItem));
			assert(sizeof(BtrfsHeader) + chunkTreeItem->offset + chunkTreeItem->size <=
				chunks[chunk].chunkItem.minIOSize); // ensure we're within bounds
			
			devices[numDevices] = *((BtrfsDevItem *)(chunkTreeBlock + sizeof(BtrfsHeader) + chunkTreeItem->offset));
			numDevices++;
		}
		else if (chunkTreeItem->key.type == 0xe4) // CHUNK_ITEM
		{
			assert((chunkTreeItem->size - sizeof(BtrfsChunkItem)) %
				sizeof(BtrfsChunkItemStripe) == 0); // ensure proper 30+20n sizing
			assert(sizeof(BtrfsHeader) + chunkTreeItem->offset + chunkTreeItem->size <=
				chunks[chunk].chunkItem.minIOSize); // ensure we're within bounds

			chunks[numChunks].logiOffset = chunkTreeItem->key.offset;
			chunks[numChunks].chunkItem =
				*((BtrfsChunkItem *)(chunkTreeBlock + sizeof(BtrfsHeader) + chunkTreeItem->offset));
			chunks[numChunks].stripes = (BtrfsChunkItemStripe *)malloc(sizeof(BtrfsChunkItemStripe) *
				chunks[numChunks].chunkItem.numStripes);
			
			for (j = 0; j < chunks[numChunks].chunkItem.numStripes; j++)
				chunks[numChunks].stripes[j] = *((BtrfsChunkItemStripe *)(chunkTreeBlock +
				sizeof(BtrfsHeader) + chunkTreeItem->offset +
				sizeof(BtrfsChunkItem) + (sizeof(BtrfsChunkItemStripe) * j)));

			numChunks++;
		}
		else if (chunkTreeItem->key.type == 0xa8) // EXTENT_ITEM
		{
			/* these are of questionable usefulness at the moment */
			printf("getChunkTree: ignoring EXTENT_ITEM for now\n");
		}
		else if (chunkTreeItem->key.type == 0xb0) // TREE_BLOCK_REF)
		{
			/* also not terribly relevant right now */
			printf("getChunkTree: ignoring TREE_BLOCK_REF for now\n");
		}
		else
			printf("getChunkTree: found an item of unexpected type in chunk tree (0x%02X)!\n", chunkTreeItem->key.type);

		chunkTreePtr += sizeof(BtrfsItem);
	}

	/* pare these arrays down to size now */
	chunks = (Chunk *)realloc(chunks, sizeof(Chunk) * numChunks);
	devices = (BtrfsDevItem *)realloc(devices, sizeof(BtrfsDevItem) * numDevices);

	/* clean up */
	free(chunkTreeBlock);

	/* dump! */
	printf("getChunkTree: dumping devices\n\n");

	for (i = 0; i < numDevices; i++)
	{
		printf("devices[%d]:\n", i);
		printf("devID         0x%016X\n", devices[i].devID);
		printf("numBytes      0x%016X\n", devices[i].numBytes);
		printf("numBytesUsed  0x%016X\n", devices[i].numBytesUsed);
		printf("bestIOAlign           0x%08X\n", devices[i].bestIOAlign);
		printf("bestIOWidth           0x%08X\n", devices[i].bestIOWidth);
		printf("minIOSize             0x%08X\n", devices[i].minIOSize);
		printf("type          0x%016X\n", devices[i].type);
		printf("generation    0x%016X\n", devices[i].generation);
		printf("startOffset   0x%016X\n", devices[i].startOffset);
		printf("devGroup              0x%08X\n", devices[i].devGroup);
		printf("seekSpeed                   0x%02X\n", devices[i].seekSpeed);
		printf("bandwidth                   0x%02X\n", devices[i].bandwidth);
		printf("devUUID         "); for (j = 0; j < 16; j++) printf("%c", devices[i].devUUID[j]); printf("\n");
		printf("fsUUID          "); for (j = 0; j < 16; j++) printf("%c", devices[i].fsUUID[j]); printf("\n\n");
	}

	printf("getChunkTree: dumping chunks\n\n");

	for (i = 0; i < numChunks; i++)
	{
		printf("chunks[%d]: ", i);
		printf("size: 0x%016X; ", chunks[i].chunkItem.chunkSize);
		printf("0x%016X -> ", chunks[i].logiOffset);
		for (j = 0; j < chunks[i].chunkItem.numStripes; j++)
			printf("%s0x%016X", (j == 0 ? "" : ", "), chunks[i].stripes[j].offset);
		printf("\n");
	}

	/* there seems to be a second chunk tree block on occasion (in btrfs_256m.img for example),
		but it currently seems to be nothing more than an incomplete copy
		(though the additional EXTENT_ITEMs/TREE_BLOCK_REFs may be different).
		unfortunately, I don't currently know how to tell if it's there just by reading the first block...
		so I'm ignoring it for now. */
}
