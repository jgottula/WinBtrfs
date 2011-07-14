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

#include <vector>
#include "btrfs_system.h"
#include "block_reader.h"
#include "crc32c.h"
#include "endian.h"
#include "roottree_parser.h"
#include "util.h"

extern std::vector<const wchar_t *> devicePaths;
extern std::vector<KeyedItem> chunkTree, rootTree;

std::vector<BlockReader *> blockReaders;
std::vector<BtrfsSuperblock> supers;
BtrfsSuperblock super; // REMOVE ME REMOVE ME
std::vector<BtrfsSBChunk *> sbChunks; // using an array of ptrs because BtrfsSBChunk is variably sized
BtrfsObjID mountedSubvol = (BtrfsObjID)0;

DWORD init()
{
	/* allocate a block reader for each device */
	std::vector<const wchar_t *>::iterator it = devicePaths.begin(), end = devicePaths.end();
	for ( ; it != end; ++it)
	{
		BlockReader *blockReader = new BlockReader(*it);

		blockReaders.push_back(blockReader);
	}
	
	return 0;
}

void cleanUp()
{
	/* iterate backwards thru the block readers and destroy them */
	for (size_t i = blockReaders.size(); i > 0; --i)
	{
		delete blockReaders.back();
		blockReaders.pop_back();
	}
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

int loadSBs()
{
	int error;
	
	/* load the superblock for each device */
	std::vector<BlockReader *>::iterator it = blockReaders.begin(), end = blockReaders.end();
	for ( ; it != end; ++it)
	{
		BlockReader *blockReader = *it;
		/* big structs on the stack; non-recursive so this shouldn't be a problem */
		BtrfsSuperblock sb1, sb2, sb3, sb4, *sbBest = &sb1;

		if ((error = blockReader->directRead(SUPERBLOCK_1_PADDR, ADDR_PHYSICAL,
			sizeof(BtrfsSuperblock), (unsigned char *)&sb1)) != ERROR_SUCCESS)
		{
			printf("readSBs: could not read a device's primary superblock!\n(Windows error code: %d)\n", error);
			return error;
		}

		/* each device's primary superblock MUST validate */
		switch ((error = validateSB(&sb1)))
		{
		case 0:
			break;
		case 1:
			printf("readSBs: a device's primary superblock is missing or invalid!\n");
			return error;
		case 2:
			printf("readSBs: a device's primary superblock checksum failed!\n");
			return error;
		default:
			printf("readSBs: a device's primary superblock failed to validate for unknown reasons!\n");
			return error;
		}

		/* try to find more a recent secondary superblock (SSD usage pattern) */

		if (blockReader->directRead(SUPERBLOCK_2_PADDR, ADDR_PHYSICAL,
			sizeof(BtrfsSuperblock), (unsigned char *)&sb2) == ERROR_SUCCESS &&
			validateSB(&sb2) == 0 && endian64(sb2.generation) > endian64(sbBest->generation))
			sbBest = &sb2;

		if (blockReader->directRead(SUPERBLOCK_3_PADDR, ADDR_PHYSICAL,
			sizeof(BtrfsSuperblock), (unsigned char *)&sb3) == ERROR_SUCCESS &&
			validateSB(&sb3) == 0 && endian64(sb3.generation) > endian64(sbBest->generation))
			sbBest = &sb3;

		if (blockReader->directRead(SUPERBLOCK_4_PADDR, ADDR_PHYSICAL,
			sizeof(BtrfsSuperblock), (unsigned char *)&sb4) == ERROR_SUCCESS &&
			validateSB(&sb4) == 0 && endian64(sb4.generation) > endian64(sbBest->generation))
			sbBest = &sb4;

		/* add the most recent superblock to the array */
		supers.push_back(*sbBest);
	}

	/* remove this as well as 'super' above */
	printf("REMOVE ME REMOVE ME\n");
	memcpy(&super, &supers[0], sizeof(BtrfsSuperblock));

	return 0;
}

int validateSB(BtrfsSuperblock *s)
{
	const char magic[8] =
	{
		'_', 'B', 'H', 'R', 'f', 'S', '_', 'M'
	};

	assert(s != NULL);

	/* magic check */
	if (memcmp(magic, s->magic, 8) != 0)
		return 1;

	/* checksum */
	if (~crc32c((unsigned int)~0, (const unsigned char *)s + sizeof(BtrfsChecksum),
		sizeof(BtrfsSuperblock) - sizeof(BtrfsChecksum)) != endian32(s->csum.crc32c))
		return 2;

	return 0;
}

void loadSBChunks(bool dump)
{
	/* using the first device's superblock here; it doesn't really matter */
	unsigned char *sbPtr = supers[0].chunkData, *sbMax = sbPtr + endian32(supers[0].n);
	BtrfsDiskKey *key;
	BtrfsSBChunk *sbChunk;
	unsigned short *numStripes;

	if (dump)
		printf("[SBChunks] n = 0x%03lx\n", endian32(supers[0].n));

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
			for (unsigned short i = 0; i < sbChunk->chunkItem.numStripes; i++)
				printf("    + STRIPE devID: 0x%I64x offset: 0x%I64x\n", endian64(sbChunk->chunkItem.stripes[i].devID),
					endian64(sbChunk->chunkItem.stripes[i].offset));
		}
	}
}

unsigned char *loadNode(unsigned __int64 blockAddr, AddrType type, BtrfsHeader **header)
{
	/* seems to be a safe assumption that all devices share the same node size */
	unsigned int blockSize = endian32(supers[0].nodeSize);
	unsigned char *nodeBlock = (unsigned char *)malloc(blockSize);
	
	printf("loadNode: warning: assuming first device!\n");

	/* this might not always be fatal, so in the future an assertion may be inappropriate */
	assert(blockReaders[0]->directRead(blockAddr, type, blockSize, nodeBlock) == 0);

	*header = (BtrfsHeader *)nodeBlock;

	/* also possibly nonfatal */
	assert(~crc32c((unsigned int)~0, nodeBlock + sizeof(BtrfsChecksum),
		blockSize - sizeof(BtrfsChecksum)) == endian32((*header)->csum.crc32c));

	return nodeBlock;
}

unsigned __int64 getTreeRootAddr(BtrfsObjID tree)
{
	unsigned __int64 addr;

	assert(parseRootTree(RTOP_GET_ADDR, &tree, &addr) == 0);

	return addr;
}

int verifyDevices()
{
	char fsUUID[0x10];
	unsigned __int64 numDevices, generation;
	
	/* check that all the devices' FS UUIDs are identical */
	std::vector<BtrfsSuperblock>::iterator it = supers.begin(), end = supers.end();
	memcpy(fsUUID, (it++)->fsUUID, 0x10);
	for (int i = 1; it != end; ++it, i++)
	{
		if (memcmp(fsUUID, it->fsUUID, 0x10) != 0)
		{
			printf("verifyDevices: the following device is not part of this Btrfs volume!\n%S\n",
				devicePaths[i]);
			return 1;
		}
	}

	/* check for duplicate devices */
	it = supers.begin();
	for ( ; it != end; ++it)
	{
		std::vector<BtrfsSuperblock>::iterator it2 = supers.begin();
		for (int i = 0; it2 != it; ++it2, i++)
		{
			if (memcmp(it->devItem.devUUID, it2->devItem.devUUID, 0x10) == 0)
			{
				printf("verifyDevices: the following device is specified more than once!\n%S\n",
					devicePaths[i]);
				return 2;
			}
		}
	}

	/* check for agreement on the number of devices in the volume */
	it = supers.begin();
	numDevices = endian64((it++)->numDevices);
	for ( ; it != end; ++it)
	{
		if (it->numDevices != numDevices)
		{
			printf("verifyDevices: there is disagreement on the number of devices in the volume!\n");
			return 3;
		}
	}
	
	/* check for the correct number of devices for the volume */
	if (blockReaders.size() < endian64(supers[0].numDevices))
	{
		printf("verifyDevices: %d too few devices given!\n",
			endian64(supers[0].numDevices) - blockReaders.size());
		return 4;
	}
	else if (blockReaders.size() > endian64(supers[0].numDevices))
	{
		/* this shouldn't ever happen, but we have an error message for it anyway */
		printf("verifyDevices: %d too many devices given!\n",
			blockReaders.size() - endian64(supers[0].numDevices));
		return 5;
	}

	/* warn the user if the superblocks' generation numbers disagree */
	it = supers.begin();
	generation = (it++)->generation;
	for ( ; it != end; ++it)
	{
		if (it->generation != generation)
		{
			printf("verifyDevices: superblock generations disagree; volume could be inconsistent!\n");
		}
	}

	return 0;
}
