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
Chunk *chunks;
int numChunks = 0;

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

DWORD readBlock(LONGLONG addr, DWORD len, LPVOID dest)
{
	LARGE_INTEGER li;
	DWORD bytesRead;

	assert(hRead != INVALID_HANDLE_VALUE);
	assert(hReadMutex != INVALID_HANDLE_VALUE);

	li.QuadPart = addr;

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

	if (crc32c(0, (const unsigned char *)s + 0x20, 0xfe0) != endian32(s->crc32))
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
	unsigned __int8 *sbPtr = super.chunks, *sbMax = super.chunks + super.n;
	BtrfsDiskKey *key;
	Chunk *chunk;
	int i;
	unsigned __int8 block[0x1000];

	while (sbPtr < sbMax)
	{
		if (sbPtr == super.chunks)
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
		
		key = (BtrfsDiskKey *)((unsigned __int8 *)sbPtr);
		sbPtr += sizeof(BtrfsDiskKey);

		assert(key->objectID == 0x100);
		assert(key->type == 0xe4);
		/* key->offset appears to be of no relevance */
		
		chunk->chunkItem = *((BtrfsChunkItem *)((unsigned __int8 *)sbPtr));
		sbPtr += sizeof(BtrfsChunkItem);

		assert(chunk->chunkItem.rootObjIDref == 2); // the format indicates that this should always be 2

		chunk->stripes = (BtrfsChunkItemStripe *)malloc(sizeof(BtrfsChunkItemStripe) * chunk->chunkItem.numStripes);

		for (i = 0; i < chunk->chunkItem.numStripes; i++)
		{
			chunk->stripes[i] = *((BtrfsChunkItemStripe *)sbPtr);
			sbPtr += sizeof(BtrfsChunkItemStripe);
		}
	}

	/* interesting addresses: 0x0040_00000, 0x0140_0000 */
}
