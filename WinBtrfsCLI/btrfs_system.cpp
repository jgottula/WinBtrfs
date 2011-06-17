/* btrfs_system.cpp
 * low-level filesystem operations
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

BlockReader *blockReader;
BtrfsSuperblock super;
std::vector<BtrfsSBChunk *> sbChunks; // using an array of ptrs because BtrfsSBChunk is variably sized
std::vector<ItemPlus> chunkTree, rootTree;

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
		ItemPlus& itemP = chunkTree.at(i);

		if (itemP.item.key.type == TYPE_CHUNK_ITEM)
		{
			BtrfsChunkItem *chunkItem = (BtrfsChunkItem *)itemP.data;

			if (logiAddr >= itemP.item.key.offset && logiAddr + len <= itemP.item.key.offset + chunkItem->chunkSize)
				/* always using the zeroth stripe for now */
				/* also assuming that everything is on the first device */
				return (logiAddr - itemP.item.key.offset) + chunkItem->stripes[0].offset;
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
	if (crc32c(0, (const unsigned char *)s + sizeof(BtrfsChecksum),
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
			printf("  CHUNK_ITEM size: 0x%I64x logi: 0x%I64x\n", endian64(sbChunk->key.offset),
				endian64(sbChunk->chunkItem.chunkSize));
			for (int i = 0; i < sbChunk->chunkItem.numStripes; i++)
				printf("    + STRIPE devID: 0x%I64x offset: 0x%I64x\n", endian64(sbChunk->chunkItem.stripes[i].devID),
					endian64(sbChunk->chunkItem.stripes[i].offset));
		}
	}
}

boost::shared_array<unsigned char> *loadNode(unsigned __int64 blockAddr, int addrType, BtrfsHeader **header)
{
	boost::shared_array<unsigned char> *sharedBlock = new boost::shared_array<unsigned char>();
	unsigned int blockSize = endian32(super.nodeSize);

	/* this might not always be fatal, so in the future an assertion may be inappropriate */
	assert(blockReader->cachedRead(blockAddr, addrType, blockSize, sharedBlock) == 0);

	*header = (BtrfsHeader *)(sharedBlock->get());

	/* also possibly nonfatal */
	assert(crc32c(0, sharedBlock->get() + sizeof(BtrfsChecksum), blockSize - sizeof(BtrfsChecksum)) ==
		endian32((*header)->csum.crc32c));

	return sharedBlock;
}

void parseChunkTreeRec(unsigned __int64 addr, CTOperation operation)
{
	unsigned char *nodeBlock, *nodePtr;
	BtrfsHeader *header;
	BtrfsItem *item;
	ItemPlus itemP;
	unsigned short *temp;
	
	boost::shared_array<unsigned char> *sharedBlock = loadNode(addr, ADDR_LOGICAL, &header);

	assert(header->tree == OBJID_CHUNK_TREE);
	
	nodeBlock = sharedBlock->get();
	nodePtr = nodeBlock + sizeof(BtrfsHeader);

	if (operation == CTOP_DUMP_TREE)
		printf("\n[Node] tree = 0x%I64x addr = 0x%I64x level = 0x%02x nrItems = 0x%08x\n", endian64(header->tree),
			addr, header->level, header->nrItems);

	if (header->level == 0) // leaf node
	{
		for (int i = 0; i < endian32(header->nrItems); i++)
		{
			item = (BtrfsItem *)nodePtr;

			if (operation == CTOP_LOAD)
			{
				switch (item->key.type)
				{
				case TYPE_DEV_ITEM:
					assert(endian32(item->size) == sizeof(BtrfsDevItem)); // ensure proper size
					assert(sizeof(BtrfsHeader) + endian32(item->offset) + endian32(item->size) <= endian32(super.nodeSize)); // ensure we're within bounds

					memcpy(&itemP.item, item, sizeof(BtrfsItem));
					itemP.data = malloc(sizeof(BtrfsDevItem));
					memcpy(itemP.data, nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset), endian32(item->size));

					chunkTree.push_back(itemP);
					break;
				case TYPE_CHUNK_ITEM:
					assert((endian32(item->size) - sizeof(BtrfsChunkItem)) % sizeof(BtrfsChunkItemStripe) == 0); // ensure proper 30+20n size
					assert(sizeof(BtrfsHeader) + endian32(item->offset) + endian32(item->size) <= endian32(super.nodeSize)); // ensure we're within bounds

					temp = (unsigned short *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset) + 0x2c);

					/* check the ACTUAL size now that we have it */
					assert(endian32(item->size) == sizeof(BtrfsChunkItem) + (*temp * sizeof(BtrfsChunkItemStripe)));

					memcpy(&itemP.item, item, sizeof(BtrfsItem));
					itemP.data = malloc(sizeof(BtrfsChunkItem) + (*temp * sizeof(BtrfsChunkItemStripe)));
					memcpy(itemP.data, nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset), endian32(item->size));

					chunkTree.push_back(itemP);
					break;
				default:
					printf("parseChunkTreeRec: don't know how to load item of type 0x%02x!\n", item->key.type);
					break;
				}
			}
			else if (operation == RTOP_DUMP_TREE)
			{
				switch (item->key.type)
				{
				case TYPE_DEV_ITEM:
				{
					BtrfsDevItem *devItem = (BtrfsDevItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));
					char uuid[1024];

					uuidToStr(devItem->devUUID, uuid);
					printf("  [%02x] DEV_ITEM devID: 0x%I64x uuid: %s\n"
						"                devGroup: 0x%lx offset: 0x%I64x size: 0x%I64x\n", i,
						endian64(item->key.offset), uuid, endian32(devItem->devGroup),
						endian64(devItem->startOffset), endian64(devItem->numBytes));
					break;
				}
				case TYPE_CHUNK_ITEM:
				{
					BtrfsChunkItem *chunkItem = (BtrfsChunkItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));
					
					printf("  [%02x] CHUNK_ITEM size: 0x%I64x logi: 0x%I64x\n", i, endian64(item->key.offset),
						endian64(chunkItem->chunkSize));
					for (int j = 0; j < chunkItem->numStripes; j++)
						printf("         + STRIPE devID: 0x%I64x offset: 0x%I64x\n", endian64(chunkItem->stripes[j].devID),
							endian64(chunkItem->stripes[j].offset));
					break;
				}
				default:
					printf("  [%02x] unknown {%I64x|%I64x}\n", i, item->key.objectID, item->key.offset);
					break;
				}
			}
			else
				printf("parseChunkTreeRec: unknown operation (0x%02x)!\n", operation);

			nodePtr += sizeof(BtrfsItem);
		}
	}
	else // non-leaf node
	{
		if (operation == CTOP_DUMP_TREE)
		{
			for (int i = 0; i < endian32(header->nrItems); i++)
			{
				BtrfsKeyPtr *keyPtr = (BtrfsKeyPtr *)(nodePtr + (sizeof(BtrfsKeyPtr) * i));

				printf("  [%02x] {%I64x|%I64x} KeyPtr: block 0x%016I64x generation 0x%016I64x\n",
					i, endian64(keyPtr->key.objectID), endian64(keyPtr->key.offset),
					endian64(keyPtr->blockNum), endian64(keyPtr->generation));
			}
		}

		for (int i = 0; i < endian32(header->nrItems); i++)
		{
			BtrfsKeyPtr *keyPtr = (BtrfsKeyPtr *)nodePtr;

			/* recurse down one level of the tree */
			parseChunkTreeRec(endian64(keyPtr->blockNum), operation);

			nodePtr += sizeof(BtrfsKeyPtr);
		}
	}

	delete sharedBlock;
}

void parseChunkTree(CTOperation operation)
{
	/* load SB chunks if we haven't already */
	if (sbChunks.size() == 0)
		loadSBChunks(true);
	
	parseChunkTreeRec(endian64(super.chunkTreeLAddr), operation);
}

void parseRootTreeRec(unsigned __int64 addr, RTOperation operation)
{
	unsigned char *nodeBlock, *nodePtr;
	BtrfsHeader *header;
	BtrfsItem *item;
	ItemPlus itemP;

	boost::shared_array<unsigned char> *sharedBlock = loadNode(addr, ADDR_LOGICAL, &header);

	assert(header->tree == OBJID_ROOT_TREE);
	
	nodeBlock = sharedBlock->get();
	nodePtr = nodeBlock + sizeof(BtrfsHeader);

	if (operation == RTOP_DUMP_TREE)
		printf("\n[Node] tree = 0x%I64x addr = 0x%I64x level = 0x%02x nrItems = 0x%08x\n", endian64(header->tree),
			addr, header->level, header->nrItems);

	if (header->level == 0) // leaf node
	{
		for (int i = 0; i < endian32(header->nrItems); i++)
		{
			item = (BtrfsItem *)nodePtr;

			if (operation == RTOP_LOAD)
			{
				switch (item->key.type)
				{
				case TYPE_ROOT_ITEM:
					assert(endian32(item->size) == sizeof(BtrfsRootItem)); // ensure proper size
					assert(sizeof(BtrfsHeader) + endian32(item->offset) + endian32(item->size) <= endian32(super.nodeSize)); // ensure we're within bounds

					memcpy(&itemP.item, item, sizeof(BtrfsItem));
					itemP.data = malloc(sizeof(BtrfsRootItem));
					memcpy(itemP.data, nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset), endian32(item->size));

					rootTree.push_back(itemP);
					break;
				default:
					printf("parseRootTreeRec: don't know how to load item of type 0x%02x!\n", item->key.type);
					break;
				}
			}
			else if (operation == RTOP_DUMP_TREE)
			{
				switch (item->key.type)
				{
				case TYPE_INODE_ITEM:
				{
					BtrfsInodeItem *inodeItem = (BtrfsInodeItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));
					char mode[11];
					
					stModeToStr(inodeItem->stMode, mode);
					printf("  [%02x] INODE_ITEM 0x%I64x uid: %d gid: %d mode: %s size: 0x%I64x\n", i,
						endian64(item->key.objectID), endian32(inodeItem->stUID), endian32(inodeItem->stGID), mode,
						endian64(inodeItem->stSize));
					break;
				}
				case TYPE_INODE_REF:
				{
					BtrfsInodeRef *inodeRef = (BtrfsInodeRef *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));
					size_t len = endian16(inodeRef->nameLen);
					char *name = new char[len + 1];

					memcpy(name, inodeRef->name, len);
					name[len] = 0;

					printf("  [%02x] INODE_REF 0x%I64x -> '%s' parent: 0x%I64x\n", i, endian64(item->key.objectID), name,
						endian64(item->key.offset));

					delete[] name;
					break;
				}
				case TYPE_DIR_ITEM:
				{
					BtrfsDirItem *dirItem = (BtrfsDirItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));
					size_t len = endian16(dirItem->n);
					char *name = new char[len + 1];

					memcpy(name, dirItem->namePlusData, len);
					name[len] = 0;

					printf("  [%02x] DIR_ITEM parent: 0x%I64x child: 0x%I64x -> '%s'\n", i, endian64(item->key.objectID),
						endian64(dirItem->child.objectID), name);
					
					delete[] name;
					break;
				}
				case TYPE_ROOT_ITEM:
				{
					BtrfsRootItem *rootItem = (BtrfsRootItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));

					printf("  [%02x] ROOT_ITEM 0x%I64x -> 0x%I64x\n", i, endian64(item->key.objectID),
						endian64(rootItem->rootNodeBlockNum));
					break;
				}
				case TYPE_ROOT_BACKREF:
					printf("  [%02x] ROOT_BACKREF subtree: 0x%I64x tree: 0x%I64x\n", i, endian64(item->key.objectID),
						endian64(item->key.offset));
					break;
				case TYPE_ROOT_REF:
					printf("  [%02x] ROOT_REF tree: 0x%I64x subtree: 0x%I64x\n", i, endian64(item->key.objectID),
						endian64(item->key.offset));
					break;
				default:
					printf("  [%02x] unknown {%I64x|%I64x}\n", i, item->key.objectID, item->key.offset);
					break;
				}
			}
			else
				printf("parseRootTreeRec: unknown operation (0x%02x)!\n", operation);

			nodePtr += sizeof(BtrfsItem);
		}
	}
	else // non-leaf node
	{
		if (operation == RTOP_DUMP_TREE)
		{
			for (int i = 0; i < endian32(header->nrItems); i++)
			{
				BtrfsKeyPtr *keyPtr = (BtrfsKeyPtr *)(nodePtr + (sizeof(BtrfsKeyPtr) * i));

				printf("  [%02x] {%I64x|%I64x} KeyPtr: block 0x%016I64x generation 0x%016I64x\n",
					i, endian64(keyPtr->key.objectID), endian64(keyPtr->key.offset),
					endian64(keyPtr->blockNum), endian64(keyPtr->generation));
			}
		}

		for (int i = 0; i < endian32(header->nrItems); i++)
		{
			BtrfsKeyPtr *keyPtr = (BtrfsKeyPtr *)nodePtr;

			/* recurse down one level of the tree */
			parseRootTreeRec(endian64(keyPtr->blockNum), operation);

			nodePtr += sizeof(BtrfsKeyPtr);
		}
	}

	delete sharedBlock;
}

void parseRootTree(RTOperation operation)
{
	parseRootTreeRec(endian64(super.rootTreeLAddr), operation);
}

void parseFSTreeRec(unsigned __int64 addr, FSOperation operation, void *input1, void *input2, void *input3,
	void *output1, void *output2, void *temp, int *returnCode, bool *shortCircuit)
{
	unsigned char *nodeBlock, *nodePtr;
	BtrfsHeader *header;
	BtrfsItem *item;

	boost::shared_array<unsigned char> *sharedBlock = loadNode(addr, ADDR_LOGICAL, &header);
	
	nodeBlock = sharedBlock->get();
	nodePtr = nodeBlock + sizeof(BtrfsHeader);

	if (operation == FSOP_DUMP_TREE)
		printf("\n[Node] tree = 0x%I64x addr = 0x%I64x level = 0x%02x nrItems = 0x%08x\n", endian64(header->tree),
			addr, header->level, header->nrItems);

	if (header->level == 0) // leaf node
	{
		for (int i = 0; i < endian32(header->nrItems); i++)
		{
			item = (BtrfsItem *)nodePtr;

			if (operation == FSOP_NAME_TO_ID)
			{
				const BtrfsObjID *parentID = (const BtrfsObjID *)input1;
				const unsigned __int64 *hash = (const unsigned __int64 *)input2;
				const char *name = (const char *)input3;
				BtrfsObjID *childID = (BtrfsObjID *)output1;
				
				/* TODO: fix hash matching */
				if (item->key.type == TYPE_DIR_ITEM && endian64(item->key.objectID) == *parentID/* &&
					endian64(item->key.offset) == *hash*/)
				{
					BtrfsDirItem *dirItem = (BtrfsDirItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));

					while (true)
					{
						/* ensure that the variably sized item fits entirely in the node block */
						assert((unsigned char *)dirItem + sizeof(BtrfsDirItem) <= (unsigned char *)nodeBlock + endian32(super.nodeSize) &&
							(unsigned char *)dirItem + sizeof(BtrfsDirItem) + endian16(dirItem->m) + endian16(dirItem->n) <=
							(unsigned char *)nodeBlock + endian32(super.nodeSize));
					
						if (endian16(dirItem->n) == strlen(name) && memcmp(dirItem->namePlusData, name, endian16(dirItem->n)) == 0)
						{
							/* found a match */
							*childID = dirItem->child.objectID;

							*returnCode = 0;
							*shortCircuit = true;
							break;
						}
					
						/* advance to the next DIR_ITEM if there are more */
						if (endian32(item->size) > sizeof(BtrfsDirItem) + endian16(dirItem->m) + endian16(dirItem->n))
						{
							dirItem = (BtrfsDirItem *)((unsigned char *)dirItem + sizeof(BtrfsDirItem) +
								endian16(dirItem->m) + endian16(dirItem->n));
						}
						else
							break;
					}
				}
			}
			else if (operation == FSOP_DUMP_TREE)
			{
				switch (item->key.type)
				{
				case TYPE_INODE_ITEM:
				{
					BtrfsInodeItem *inodeItem = (BtrfsInodeItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));
					char mode[11];
					
					stModeToStr(inodeItem->stMode, mode);
					printf("  [%02x] INODE_ITEM 0x%I64x uid: %d gid: %d mode: %s size: 0x%I64x\n", i,
						endian64(item->key.objectID), endian32(inodeItem->stUID), endian32(inodeItem->stGID), mode,
						endian64(inodeItem->stSize));
					break;
				}
				case TYPE_INODE_REF:
				{
					BtrfsInodeRef *inodeRef = (BtrfsInodeRef *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));
					size_t len = endian16(inodeRef->nameLen);
					char *name = new char[len + 1];

					memcpy(name, inodeRef->name, len);
					name[len] = 0;

					printf("  [%02x] INODE_REF 0x%I64x -> '%s' parent: 0x%I64x\n", i, endian64(item->key.objectID), name,
						endian64(item->key.offset));

					delete[] name;
					break;
				}
				case TYPE_DIR_ITEM:
				{
					BtrfsDirItem *dirItem = (BtrfsDirItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));
					size_t len = endian16(dirItem->n);
					char *name = new char[len + 1];

					memcpy(name, dirItem->namePlusData, len);
					name[len] = 0;

					printf("  [%02x] DIR_ITEM parent: 0x%I64x child: 0x%I64x -> '%s'\n", i, endian64(item->key.objectID),
						endian64(dirItem->child.objectID), name);
					
					delete[] name;
					break;
				}
				case TYPE_DIR_INDEX:
					printf("  [%02x] DIR_INDEX 0x%I64x = idx 0x%I64x\n", i, endian64(item->key.objectID),
						endian64(item->key.offset));
					break;
				case TYPE_EXTENT_DATA:
				{
					BtrfsExtentData *extentData = (BtrfsExtentData *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));

					printf("  [%02x] EXTENT_DATA 0x%I64x offset: 0x%I64x size: 0x%I64x type: %s\n", i,
						endian64(item->key.objectID), endian64(item->key.offset), endian64(extentData->n),
						(extentData->type == 0 ? "inline" : (extentData->type == 1 ? "regular" : "prealloc")));
					break;
				}
				default:
					printf("  [%02x] unknown {%I64x|%I64x}\n", i, item->key.objectID, item->key.offset);
					break;
				}
			}
			else if (operation == FSOP_GET_FILE_PKG)
			{
				const BtrfsObjID *objectID = (const BtrfsObjID *)input1;
				FilePkg *filePkg = (FilePkg *)output1;

				if (item->key.type == TYPE_INODE_ITEM && endian64(item->key.objectID) == *objectID) // inode
				{
					BtrfsInodeItem *inodeItem = (BtrfsInodeItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));
					
					/* ensure that the item fits entirely in the node block */
					assert((unsigned char *)inodeItem + sizeof(BtrfsInodeItem) <= (unsigned char *)nodeBlock + endian32(super.nodeSize));

					memcpy(&(filePkg->inode), inodeItem, sizeof(BtrfsInodeItem));
					
					*returnCode &= ~0x1; // clear bit 0

					if (*returnCode == 0)
						*shortCircuit = true;
				}
				else if (item->key.type == TYPE_DIR_ITEM) // name & parent
				{
					BtrfsDirItem *dirItem = (BtrfsDirItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));
					
					while (true)
					{
						/* ensure that the variably sized item fits entirely in the node block */
						assert((unsigned char *)dirItem + sizeof(BtrfsDirItem) <= (unsigned char *)nodeBlock + endian32(super.nodeSize) &&
							(unsigned char *)dirItem + sizeof(BtrfsDirItem) + endian16(dirItem->m) + endian16(dirItem->n) <=
							(unsigned char *)nodeBlock + endian32(super.nodeSize));
						
						if (endian64(dirItem->child.objectID) == *objectID) // parent info
						{
							memcpy(filePkg->name, dirItem->namePlusData,
								(endian16(dirItem->n) <= 255 ? endian16(dirItem->n) : 255)); // limit to 255
							filePkg->name[endian16(dirItem->n)] = 0;

							filePkg->parentID = (BtrfsObjID)endian64(item->key.objectID);
							
							*returnCode &= ~0x2; // clear bit 1

							if (*returnCode == 0)
								*shortCircuit = true;
						}
						
						/* advance to the next DIR_ITEM if there are more */
						if (endian32(item->size) > sizeof(BtrfsDirItem) + endian16(dirItem->m) + endian16(dirItem->n))
						{
							dirItem = (BtrfsDirItem *)((unsigned char *)dirItem + sizeof(BtrfsDirItem) +
								endian16(dirItem->m) + endian16(dirItem->n));
						}
						else
							break;
					}
				}
			}
			else if (operation == FSOP_DIR_LIST)
			{
				const BtrfsObjID *objectID = (const BtrfsObjID *)input1;
				DirList *dirList = (DirList *)output1;
				
				if (item->key.type == TYPE_INODE_ITEM) // inode
				{
					BtrfsInodeItem *inodeItem = (BtrfsInodeItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));
					
					/* ensure that the item fits entirely in the node block */
					assert((unsigned char *)inodeItem + sizeof(BtrfsInodeItem) <= (unsigned char *)nodeBlock + endian32(super.nodeSize));

					if (dirList->numEntries == (*objectID == OBJID_ROOT_DIR ? 0 : 1)) // no entries have been created yet
					{
						/* save this inode for later in case it happens to be the inode associated with '..' */
						memcpy(temp, inodeItem, sizeof(BtrfsInodeItem));
					}

					for (int j = 0; j < dirList->numEntries; j++) // try to find a matching entry
					{
						if (endian64(item->key.objectID) == dirList->entries[j].objectID)
						{
							memcpy(&(dirList->entries[j].inode), inodeItem, sizeof(BtrfsInodeItem));

							(*returnCode)--;
							break;
						}
					}
				}
				else if (item->key.type == TYPE_DIR_ITEM) // allocate, ID, parent, name
				{
					BtrfsDirItem *dirItem = (BtrfsDirItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));
					
					while (true)
					{
						/* ensure that the variably sized item fits entirely in the node block */
						assert((unsigned char *)dirItem + sizeof(BtrfsDirItem) <= (unsigned char *)nodeBlock + endian32(super.nodeSize) &&
							(unsigned char *)dirItem + sizeof(BtrfsDirItem) + endian16(dirItem->m) + endian16(dirItem->n) <=
							(unsigned char *)nodeBlock + endian32(super.nodeSize));
						
						if (endian64(item->key.objectID) == *objectID)
						{
							if (dirList->entries == NULL)
								dirList->entries = (FilePkg *)malloc(sizeof(FilePkg));
							else
								dirList->entries = (FilePkg *)realloc(dirList->entries, sizeof(FilePkg) * (dirList->numEntries + 1));

							dirList->entries[dirList->numEntries].objectID = (BtrfsObjID)endian64(dirItem->child.objectID);
							dirList->entries[dirList->numEntries].parentID = (BtrfsObjID)endian64(item->key.objectID);

							memcpy(dirList->entries[dirList->numEntries].name, dirItem->namePlusData,
								(endian16(dirItem->n) <= 255 ? endian16(dirItem->n) : 255)); // limit to 255
							dirList->entries[dirList->numEntries].name[endian16(dirItem->n)] = 0;

							dirList->numEntries++;

							(*returnCode)++;
						}
						
						/* special case for '..' */
						if (*objectID != OBJID_ROOT_DIR && endian64(dirItem->child.objectID) == *objectID)
						{
							if (dirList->entries == NULL)
								dirList->entries = (FilePkg *)malloc(sizeof(FilePkg));
							else
								dirList->entries = (FilePkg *)realloc(dirList->entries, sizeof(FilePkg) * (dirList->numEntries + 1));

							/* go back and assign the parent for '.' since we have that value handy */
							/* this assumes that the first entry is always '.' for non-root dirs, which is currently
								always the case. */
							dirList->entries[0].parentID = (BtrfsObjID)endian64(item->key.objectID);

							dirList->entries[dirList->numEntries].objectID = (BtrfsObjID)endian64(item->key.objectID);
							/* not currently assigning parentID, as it's unnecessary and not needed by the dir list callback */

							strcpy(dirList->entries[dirList->numEntries].name, "..");

							/* grab the inode we saved earlier and shove it in */
							memcpy(&(dirList->entries[dirList->numEntries].inode), temp, sizeof(BtrfsInodeItem));

							dirList->numEntries++;
						}
						
						/* advance to the next DIR_ITEM if there are more */
						if (endian32(item->size) > sizeof(BtrfsDirItem) + endian16(dirItem->m) + endian16(dirItem->n))
						{
							dirItem = (BtrfsDirItem *)((unsigned char *)dirItem + sizeof(BtrfsDirItem) +
								endian16(dirItem->m) + endian16(dirItem->n));
						}
						else
							break;
					}
				}
			}
			else
				printf("parseFSTreeRec: unknown operation (0x%02x)!\n", operation);

			if (*shortCircuit)
				break;

			nodePtr += sizeof(BtrfsItem);
		}
	}
	else // non-leaf node
	{
		if (operation == FSOP_DUMP_TREE)
		{
			for (int i = 0; i < endian32(header->nrItems); i++)
			{
				BtrfsKeyPtr *keyPtr = (BtrfsKeyPtr *)(nodePtr + (sizeof(BtrfsKeyPtr) * i));

				printf("  [%02x] {%I64x|%I64x} KeyPtr: block 0x%016I64x generation 0x%016I64x\n",
					i, endian64(keyPtr->key.objectID), endian64(keyPtr->key.offset),
					endian64(keyPtr->blockNum), endian64(keyPtr->generation));
			}
		}

		for (int i = 0; i < endian32(header->nrItems); i++)
		{
			BtrfsKeyPtr *keyPtr = (BtrfsKeyPtr *)nodePtr;

			/* recurse down one level of the tree */
			parseFSTreeRec(endian64(keyPtr->blockNum), operation, input1, input2, input3, output1, output2,
				temp, returnCode, shortCircuit);

			if (*shortCircuit)
				break;

			nodePtr += sizeof(BtrfsKeyPtr);
		}
	}

	delete sharedBlock;
}

int parseFSTree(FSOperation operation, void *input1, void *input2, void *input3, void *output1, void *output2)
{
	unsigned __int64 addr;
	int returnCode;
	bool shortCircuit = false;
	BtrfsInodeItem inode;
	
	switch (operation)
	{
	case FSOP_DUMP_TREE:		// always succeeds
	case FSOP_DIR_LIST:			// begins at zero for other reasons
		returnCode = 0;
		break;
	case FSOP_GET_FILE_PKG:
		returnCode = 0x1; // always need the inode
		if (*((BtrfsObjID *)input1) != OBJID_ROOT_DIR)
			returnCode |= 0x2; // need parent & name for all except the root dir
		break;
	default:
		returnCode = 0x1; // 1 bit = 1 part MUST be fulfilled
	}
	
	/* pre tasks */
	if (operation == FSOP_GET_FILE_PKG)
	{
		const BtrfsObjID *objectID = (const BtrfsObjID *)input1;
		FilePkg *filePkg = (FilePkg *)output1;

		filePkg->objectID = *objectID;

		/* for the special case of the root dir, this stuff wouldn't get filled in by any other means */
		if (*objectID == OBJID_ROOT_DIR)
		{
			strcpy(filePkg->name, "ROOT_DIR");
			filePkg->parentID = (BtrfsObjID)0x0;
		}
	}
	else if (operation == FSOP_DIR_LIST)
	{
		const BtrfsObjID *objectID = (const BtrfsObjID *)input1;
		DirList *dirList = (DirList *)output1;

		if (*objectID != OBJID_ROOT_DIR)
		{
			dirList->numEntries = 1;
			dirList->entries = (FilePkg *)malloc(sizeof(FilePkg));

			/* add '.' to the list */
			dirList->entries[0].objectID = *objectID;
			strcpy(dirList->entries[0].name, ".");

			returnCode++;
		}
		else
		{
			dirList->numEntries = 0;
			dirList->entries = NULL;
		}
	}

	if (operation == FSOP_DUMP_TREE && input1 != NULL)
		addr = *((unsigned __int64 *)input1);
	else
		addr = getFSRootBlockNum();
	
	parseFSTreeRec(addr, operation, input1, input2, input3, output1, output2,
		(operation == FSOP_DIR_LIST ? &inode : NULL), &returnCode, &shortCircuit);

	if (operation == FSOP_GET_FILE_PKG)
	{
		FilePkg *filePkg = (FilePkg *)output1;

		if (returnCode == 0)
		{
			if (filePkg->name[0] == '.' && strcmp(filePkg->name, ".") != 0 && strcmp(filePkg->name, "..") != 0)
				filePkg->hidden = true;
			else
				filePkg->hidden = false;
		}
	}
	else if (operation == FSOP_DIR_LIST)
	{
		const BtrfsObjID *objectID = (const BtrfsObjID *)input1;
		DirList *dirList = (DirList *)output1;

		if (returnCode == 0)
		{
			for (int i = 0; i < dirList->numEntries; i++)
			{
				if (dirList->entries[i].name[0] == '.' && strcmp(dirList->entries[i].name, ".") != 0 &&
					strcmp(dirList->entries[i].name, "..") != 0)
					dirList->entries[i].hidden = true;
				else
					dirList->entries[i].hidden = false;
			}
		}
		else
			free(dirList->entries);
	}

	return returnCode;
}

unsigned __int64 getFSRootBlockNum()
{
	/* no way we can find the FS root node without the root tree */
	assert(rootTree.size() > 0);
	
	size_t size = rootTree.size();
	for (size_t i = 0; i < size; i++)
	{
		ItemPlus& itemP = rootTree.at(i);

		if (itemP.item.key.type == TYPE_ROOT_ITEM && endian64(itemP.item.key.objectID) == OBJID_FS_TREE)
		{
			BtrfsRootItem *rootItem = (BtrfsRootItem *)itemP.data;

			return endian64(rootItem->rootNodeBlockNum);
		}
	}

	/* getting here means we couldn't find it */
	assert(0);
}
