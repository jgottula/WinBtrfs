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
#include "btrfs_filesystem.h"

extern WCHAR devicePath[MAX_PATH];

HANDLE hRead = INVALID_HANDLE_VALUE, hReadMutex = INVALID_HANDLE_VALUE;
BtrfsSuperblock super;
BtrfsDevItem *devices = NULL;
int numDevices = -1;
Chunk *chunks = NULL;
int numChunks = -1;
Root *roots = NULL;
int numRoots = -1;

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

unsigned __int64 logiToPhys(unsigned __int64 logiAddr, unsigned __int64 len)
{
	int i, chunk = -1;

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
	return (logiAddr - chunks[chunk].logiOffset) + chunks[chunk].stripes[0].offset;
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
	return readBlock(logiToPhys(logiAddr, len), len, dest);
}

DWORD readPrimarySB()
{
	return readBlock(0x10000, sizeof(BtrfsSuperblock), &super);
}

int validateSB(BtrfsSuperblock *s)
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
	BtrfsSuperblock s2, s3, s4, *sBest = &super;
	int best = 1;

	/* read each superblock (if present) and validate */

	if (readBlock(0x4000000, sizeof(BtrfsSuperblock), &s2) == 0 && validateSB(&s2) == 0 &&
		s2.generation > sBest->generation)
		best = 2, sBest = &s2;

	if (readBlock(0x4000000000, sizeof(BtrfsSuperblock), &s3) == 0 && validateSB(&s3) == 0 &&
		s3.generation > sBest->generation)
		best = 3, sBest = &s3;

	if (readBlock(0x4000000000000, sizeof(BtrfsSuperblock), &s4) == 0 && validateSB(&s4) == 0 &&
		s4.generation > sBest->generation)
		best = 4, sBest = &s4;

	/* replace the superblock in memory with the most up-to-date on-disk copy */

	if (best != 1)
		memcpy(&super, sBest, sizeof(BtrfsSuperblock));

	return best;
}

void getSBChunks()
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

void parseNodePhysical(unsigned __int64 physAddr)
{
	unsigned char *nodeBlock, *nodePtr;
	BtrfsHeader *header;
	BtrfsItem *item;
	int i, j;

	nodePtr = nodeBlock = (unsigned char *)malloc(super.nodeSize);

	/* this might not always be fatal, so an assertion may be inappropriate */
	assert(readBlock(physAddr, super.nodeSize, nodeBlock) == 0);

	header = (BtrfsHeader *)nodePtr;
	nodePtr += sizeof(BtrfsHeader);

	/* again, this may not warrant stopping the whole program if reading from a less important tree */
	assert(crc32c(0, nodeBlock + 0x20, super.nodeSize - 0x20) == endian32(header->crc32c));

	/* pre tasks */
	switch (header->tree)
	{
	case 0x01: // root tree
		break;
	case 0x03: // chunk tree
		break;
	case 0x05: // fs tree
		break;
	default:
		printf("parseNode: given a tree of an unknown type [0x%02x]!\n", header->tree);
		break;
	}

	for (i = 0; i < header->nrItems; i++)
	{
		item = (BtrfsItem *)nodePtr;

		switch (item->key.type)
		{
		case 0x84: // ROOT_ITEM
			if (header->tree != 0x01)
			{
				printf("parseNode: ROOT_ITEM unexpected in tree of type 0x%02x!\n", header->tree);
				break;
			}

			assert(item->size == sizeof(BtrfsRootItem)); // ensure proper size
			assert(sizeof(BtrfsHeader) + item->offset + item->size <= super.nodeSize); // ensure we're within bounds

			if (numRoots == -1)
			{
				roots = (Root *)malloc(sizeof(Root));
				numRoots = 0;
			}
			else
				roots = (Root *)realloc(roots, sizeof(Root) * (numRoots + 1));

			roots[numRoots].objectID = item->key.objectID;
			roots[numRoots].rootItem = *((BtrfsRootItem *)(nodeBlock + sizeof(BtrfsHeader) + item->offset));

			numRoots++;
			break;
		case 0xd8: // DEV_ITEM
			if (header->tree != 0x03)
			{
				printf("parseNode: DEV_ITEM unexpected in tree of type 0x%02x!\n", header->tree);
				break;
			}

			assert(item->size == sizeof(BtrfsDevItem)); // ensure proper size
			assert(sizeof(BtrfsHeader) + item->offset + item->size <= super.nodeSize); // ensure we're within bounds

			if (numDevices == -1)
			{
				devices = (BtrfsDevItem *)malloc(sizeof(BtrfsDevItem));
				numDevices = 0;
			}
			else
				devices = (BtrfsDevItem *)realloc(devices, sizeof(BtrfsDevItem) * (numDevices + 1));

			devices[numDevices] = *((BtrfsDevItem *)(nodeBlock + sizeof(BtrfsHeader) + item->offset));

			numDevices++;
			break;
		case 0xe4: // CHUNK_ITEM
			if (header->tree != 0x03)
			{
				printf("parseNode: CHUNK_ITEM unexpected in tree of type 0x%02x!\n", header->tree);
				break;
			}

			assert((item->size - sizeof(BtrfsChunkItem)) % sizeof(BtrfsChunkItemStripe) == 0); // ensure proper 30+20n size
			assert(sizeof(BtrfsHeader) + item->offset + item->size <= super.nodeSize); // ensure we're within bounds

			if (numChunks == -1)
			{
				chunks = (Chunk *)malloc(sizeof(Chunk));
				numChunks = 0;
			}
			else
				chunks = (Chunk *)realloc(chunks, sizeof(Chunk) * (numChunks + 1));

			chunks[numChunks].logiOffset = item->key.offset;
			chunks[numChunks].chunkItem = *((BtrfsChunkItem *)(nodeBlock + sizeof(BtrfsHeader) + item->offset));
			chunks[numChunks].stripes = (BtrfsChunkItemStripe *)malloc(sizeof(BtrfsChunkItemStripe) *
				chunks[numChunks].chunkItem.numStripes);

			for (j = 0; j < chunks[numChunks].chunkItem.numStripes; j++)
				chunks[numChunks].stripes[j] = *((BtrfsChunkItemStripe *)(nodeBlock + sizeof(BtrfsHeader) + item->offset +
					sizeof(BtrfsChunkItem) + (sizeof(BtrfsChunkItemStripe) * j)));

			numChunks++;
			break;
		default:
			printf("parseNode: found an item of unrecognized type [0x%02x] in tree of type 0x%02x!\n",
				item->key.type, header->tree);
			break;
		}

		nodePtr += sizeof(BtrfsItem);
	}

	/* post tasks */
	switch (header->tree)
	{
	case 0x01: // root tree
		break;
	case 0x03: // chunk tree
		break;
	case 0x05: // fs tree
		break;
	}

	/* clean up */
	free(nodeBlock);
}

void parseNodeLogical(unsigned __int64 logiAddr)
{
	parseNodePhysical(logiToPhys(logiAddr, super.nodeSize));
}

void parseChunkTree()
{
	unsigned __int64 chunkTreeLogiAddr = super.chunkTreeLAddr;
	int i, j, chunk = -1, stripe = -1;

	/* we need the superblock chunks first so we can bootstrap to the chunk tree */
	getSBChunks();

	/* find a superblock chunk for the location of the chunk tree so we can load it in */
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
	
	parseNodePhysical(chunks[chunk].stripes[stripe].offset + (chunkTreeLogiAddr - chunks[chunk].logiOffset));
}

void parseRootTree()
{
	parseNodeLogical(super.rootTreeLAddr);
}

void parseFSTree()
{
	int i, root = -1;

	/* no way we can find the FS root node without the root tree */
	assert(numRoots > 0 && roots != NULL);
	
	for (i = 0; i < numRoots; i++)
	{
		if (roots[i].objectID == 0x05)
		{
			root = i;
			break;
		}
	}

	assert(root != -1);

	parseNodeLogical(roots[root].rootItem.rootNodeBlockNum);
}

void dump()
{
	int i, j;

	/* take a dump */
	printf("dump: dumping devices\n\n");
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

	printf("dump: dumping chunks\n\n");
	for (i = 0; i < numChunks; i++)
	{
		printf("chunks[%d]: ", i);
		printf("size: 0x%016X; ", chunks[i].chunkItem.chunkSize);
		printf("0x%016X -> ", chunks[i].logiOffset);
		for (j = 0; j < chunks[i].chunkItem.numStripes; j++)
			printf("%s0x%016X", (j == 0 ? "" : ", "), chunks[i].stripes[j].offset);
		printf("\n");
	}

	printf("dump: dumping roots\n\n");
	for (i = 0; i < numRoots; i++)
	{
		printf("roots[%d]:\n", i);
		printf("[objectID]          0x%016X\n", roots[i].objectID);
		printf("inodeItem                          ...\n");
		printf("expectedGeneration  0x%016X\n", roots[i].rootItem.expectedGeneration);
		printf("objID               0x%016X\n", roots[i].rootItem.objID);
		printf("rootNodeBlockNum    0x%016X\n", roots[i].rootItem.rootNodeBlockNum);
		printf("byteLimit           0x%016X\n", roots[i].rootItem.byteLimit);
		printf("bytesUsed           0x%016X\n", roots[i].rootItem.bytesUsed);
		printf("lastGenSnapshot     0x%016X\n", roots[i].rootItem.lastGenSnapshot);
		printf("flags               0x%016X\n", roots[i].rootItem.flags);
		printf("numRefs                     0x%08X\n", roots[i].rootItem.numRefs);
		printf("dropProgress                       ...\n");
		printf("dropLevel                         0x%02X\n", roots[i].rootItem.dropLevel);
		printf("rootLevel                         0x%02X\n\n", roots[i].rootItem.rootLevel);
	}
}
