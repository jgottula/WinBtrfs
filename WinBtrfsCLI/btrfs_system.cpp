/* btrfs_system.cpp
 * low-level filesystem code
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
#include <vector>
#include <Windows.h>
#include "structures.h"
#include "constants.h"
#include "util.h"
#include "endian.h"
#include "crc32c.h"
#include "btrfs_system.h"
#include "block_reader.h"

extern std::vector<KeyedItem> chunkTree, rootTree;

BlockReader *blockReader;
BtrfsSuperblock super;
std::vector<BtrfsSBChunk *> sbChunks; // using an array of ptrs because BtrfsSBChunk is variably sized
BtrfsObjID defaultSubvol = (BtrfsObjID)0;

DWORD init()
{
	blockReader = new BlockReader();
	
	return 0;
}

void cleanUp()
{
	delete blockReader;
}

unsigned __int64 logiToPhys(unsigned __int64 logiAddr, unsigned __int64 len)
{
	/* try superblock chunks first */
	size_t size = sbChunks.size();
	for (size_t i = 0; i < size; i++)
	{
		BtrfsSBChunk *chunk = sbChunks.at(i);
		
		if (logiAddr >= chunk->key.offset && logiAddr + len <= chunk->key.offset + chunk->chunkItem.chunkSize)
			/* always using the zeroth stripe for now */
			/* also assuming that everything is on the first device */
			return (logiAddr - chunk->key.offset) + chunk->chunkItem.stripes[0].offset;
	}

	/* next try the chunk tree */
	size = chunkTree.size();
	for (size_t i = 0; i < size; i++)
	{
		KeyedItem& kItem = chunkTree.at(i);

		if (kItem.key.type == TYPE_CHUNK_ITEM)
		{
			BtrfsChunkItem *chunkItem = (BtrfsChunkItem *)kItem.data;

			if (logiAddr >= kItem.key.offset && logiAddr + len <= kItem.key.offset + chunkItem->chunkSize)
				/* always using the zeroth stripe for now */
				/* also assuming that everything is on the first device */
				return (logiAddr - kItem.key.offset) + chunkItem->stripes[0].offset;
		}
	}

	/* if flow gets here, it means we failed */
	assert(0);
}

DWORD readPrimarySB()
{
	return blockReader->directRead(SUPERBLOCK_1_PADDR, ADDR_PHYSICAL, sizeof(BtrfsSuperblock), (unsigned char *)&super);
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
	if (~crc32c((unsigned int)~0, (const unsigned char *)s + sizeof(BtrfsChecksum),
		sizeof(BtrfsSuperblock) - sizeof(BtrfsChecksum)) != endian32(s->csum.crc32c))
		return 2;

	return 0;
}

int findSecondarySBs()
{
	BtrfsSuperblock s2, s3, s4, *sBest = &super;
	int best = 1;
	unsigned __int64 bestGen = endian64(super.generation);

	/* read each superblock (if present) and validate */

	if (blockReader->directRead(SUPERBLOCK_2_PADDR, ADDR_PHYSICAL, sizeof(BtrfsSuperblock), (unsigned char *)&s2) == 0 &&
		validateSB(&s2) == 0 && endian64(s2.generation) > bestGen)
		best = 2, sBest = &s2, bestGen = endian64(s2.generation);

	if (blockReader->directRead(SUPERBLOCK_2_PADDR, ADDR_PHYSICAL, sizeof(BtrfsSuperblock), (unsigned char *)&s3) == 0 &&
		validateSB(&s3) == 0 && endian64(s3.generation) > bestGen)
		best = 3, sBest = &s3, bestGen = endian64(s3.generation);

	if (blockReader->directRead(SUPERBLOCK_2_PADDR, ADDR_PHYSICAL, sizeof(BtrfsSuperblock), (unsigned char *)&s4) == 0 &&
		validateSB(&s4) == 0 && endian64(s4.generation) > bestGen)
		best = 4, sBest = &s4, bestGen = endian64(s4.generation);

	/* replace the superblock in memory with the most up-to-date on-disk copy */
	if (best != 1)
	{
		printf("findSecondarySBs: found a better superblock (#%d).\n", best);
		memcpy(&super, sBest, sizeof(BtrfsSuperblock));
	}

	return best;
}

void loadSBChunks(bool dump)
{
	unsigned char *sbPtr = super.chunkData, *sbMax = super.chunkData + endian32(super.n);
	BtrfsDiskKey *key;
	BtrfsSBChunk *sbChunk;
	unsigned short *numStripes;

	if (dump)
		printf("[SBChunks] n = 0x%03lx\n", endian32(super.n));

	while (sbPtr < sbMax)
	{
		key = (BtrfsDiskKey *)((unsigned char *)sbPtr);

		/* CHUNK_ITEMs should ALWAYS fit these constraints */
		assert(endian64(key->objectID) == 0x100);
		assert(key->type == TYPE_CHUNK_ITEM);

		/* allocate variably sized struct using information we shouldn't have yet */
		numStripes = (unsigned short *)(sbPtr + 0x3d);
		sbChunk = (BtrfsSBChunk *)malloc(sizeof(BtrfsSBChunk) + (*numStripes * sizeof(BtrfsChunkItemStripe)));

		memcpy(sbChunk, sbPtr, sizeof(BtrfsSBChunk) + (*numStripes * sizeof(BtrfsChunkItemStripe)));
		sbPtr += sizeof(BtrfsSBChunk) + (*numStripes * sizeof(BtrfsChunkItemStripe));

		sbChunks.push_back(sbChunk);

		if (dump)
		{
			char type[32];
			
			bgFlagsToStr((BlockGroupFlags)endian64(sbChunk->chunkItem.type), type);
			printf("  CHUNK_ITEM size: 0x%I64x logi: 0x%I64x type: %s\n", endian64(sbChunk->chunkItem.chunkSize),
				endian64(sbChunk->key.offset), type);
			for (int i = 0; i < sbChunk->chunkItem.numStripes; i++)
				printf("    + STRIPE devID: 0x%I64x offset: 0x%I64x\n", endian64(sbChunk->chunkItem.stripes[i].devID),
					endian64(sbChunk->chunkItem.stripes[i].offset));
		}
	}
}

unsigned char *loadNode(unsigned __int64 blockAddr, AddrType type, BtrfsHeader **header)
{
	unsigned int blockSize = endian32(super.nodeSize);
	unsigned char *nodeBlock = (unsigned char *)malloc(blockSize);

	/* this might not always be fatal, so in the future an assertion may be inappropriate */
	DWORD result = blockReader->directRead(blockAddr, type, blockSize, nodeBlock);
	assert(result == 0);

	*header = (BtrfsHeader *)nodeBlock;

	/* also possibly nonfatal */
	assert(~crc32c((unsigned int)~0, nodeBlock + sizeof(BtrfsChecksum),
		blockSize - sizeof(BtrfsChecksum)) == endian32((*header)->csum.crc32c));

	return nodeBlock;
}

unsigned __int64 getTreeRootAddr(BtrfsObjID tree)
{
	/* the root tree MUST be loaded */
	assert(rootTree.size() > 0);
	
	size_t size = rootTree.size();
	for (size_t i = 0; i < size; i++)
	{
		KeyedItem& kItem = rootTree.at(i);

		if (kItem.key.type == TYPE_ROOT_ITEM && endian64(kItem.key.objectID) == tree)
		{
			BtrfsRootItem *rootItem = (BtrfsRootItem *)kItem.data;

			return endian64(rootItem->rootNodeBlockNum);
		}
	}

	/* getting here means we couldn't find it */
	assert(0);
}
