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

#include <assert.h>
#include <Windows.h>
#include "structures.h"
#include "endian.h"
#include "crc32c.h"

extern WCHAR devicePath[MAX_PATH];

HANDLE hRead = INVALID_HANDLE_VALUE, hReadMutex = INVALID_HANDLE_VALUE;
Superblock super;
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
	/* this requires that the chunk tree has been loaded! */

	/* we need to ensure that the ENTIRE block fits within a particular chunk mapping.
		if not, we either need to (a) automatically figure out the chunks the block bridges, or
		(b) send an error back to the caller. */
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

	if (s == NULL)
		s = &super;

	if (memcmp(magic, s->magic, 8) != 0)
		return 1;

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

		assert(key->objectID == 0x100);
		assert(key->type == 0xe4);
		
		chunk->logiOffset = key->offset;
		
		chunk->chunkItem = *((BtrfsChunkItem *)((unsigned char *)sbPtr));
		sbPtr += sizeof(BtrfsChunkItem);

		assert(chunk->chunkItem.rootObjIDref == 2); // the format indicates that this should always be 2

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
	unsigned char *chunkTreeBlock;

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

	/* we MUST have a mapping for the chunk containing the chunk tree! */
	assert(chunk != -1 && stripe != -1);

	chunkTreeBlock = (unsigned char *)malloc(chunks[chunk].chunkItem.minIOSize);
	assert(readBlock(chunks[chunk].stripes[stripe].offset + (chunkTreeLogiAddr - chunks[chunk].logiOffset),
		chunks[chunk].chunkItem.minIOSize, chunkTreeBlock) == 0);

	
}
