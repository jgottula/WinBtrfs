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
#include <Windows.h>
#include "structures.h"
#include "constants.h"
#include "endian.h"
#include "crc32c.h"
#include "btrfs_system.h"
#include "block_reader.h"

BlockReader *blockReader;
BtrfsSuperblock super;
BtrfsDevItem *devices = NULL;
int numDevices = -1;
Chunk *chunks = NULL;
int numChunks = -1;
Root *roots = NULL;
int numRoots = -1;

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

void getSBChunks()
{
	unsigned char *sbPtr = super.chunkData, *sbMax = super.chunkData + endian32(super.n);
	BtrfsDiskKey *key;
	Chunk *chunk;
	int i;

	/* this function only needs to be run ONCE */
	assert(chunks == NULL && numChunks == -1);

	while (sbPtr < sbMax)
	{
		if (sbPtr == super.chunkData) // first iteration
		{
			numChunks = 1;
			chunks = (Chunk *)malloc(sizeof(Chunk));
		}
		else // subsequent iterations
		{
			numChunks++;
			chunks = (Chunk *)realloc(chunks, sizeof(Chunk) * numChunks);
		}

		chunk = &(chunks[numChunks - 1]);
		
		key = (BtrfsDiskKey *)((unsigned char *)sbPtr);
		sbPtr += sizeof(BtrfsDiskKey);

		/* CHUNK_ITEMs should ALWAYS fit these constraints */
		assert(endian64(key->objectID) == 0x100);
		assert(key->type == TYPE_CHUNK_ITEM);
		
		chunk->logiOffset = endian64(key->offset);
		
		chunk->chunkItem = *((BtrfsChunkItem *)((unsigned char *)sbPtr));
		sbPtr += sizeof(BtrfsChunkItem);

		chunk->stripes = (BtrfsChunkItemStripe *)malloc(sizeof(BtrfsChunkItemStripe) * endian16(chunk->chunkItem.numStripes));

		for (i = 0; i < endian16(chunk->chunkItem.numStripes); i++)
		{
			chunk->stripes[i] = *((BtrfsChunkItemStripe *)sbPtr);
			sbPtr += sizeof(BtrfsChunkItemStripe);
		}
	}
}

void loadNode(unsigned __int64 blockAddr, int addrType, unsigned char **nodeDest, BtrfsHeader **header)
{
	unsigned int blockSize = endian32(super.nodeSize);
	
	*nodeDest = (unsigned char *)malloc(blockSize);

	/* this might not always be fatal, so in the future an assertion may be inappropriate */
	assert(blockReader->cachedRead(blockAddr, addrType, blockSize, *nodeDest) == 0);

	*header = (BtrfsHeader *)(*nodeDest);

	/* also possibly nonfatal */
	assert(crc32c(0, *nodeDest + sizeof(BtrfsChecksum), blockSize - sizeof(BtrfsChecksum)) == endian32((*header)->csum.crc32c));
}

void parseChunkTreeRec(unsigned __int64 addr)
{
	unsigned char *nodeBlock, *nodePtr;
	BtrfsHeader *header;
	BtrfsItem *item;
	int i, j;

	loadNode(addr, ADDR_LOGICAL, &nodeBlock, &header);

	assert(header->tree == OBJID_CHUNK_TREE);
	
	nodePtr = nodeBlock + sizeof(BtrfsHeader);

	/* clear out the superblock chunk items to make way for the chunk tree chunk items */
	for (i = 0; i < numChunks; i++)
		free(chunks[i].stripes);
	numChunks = 0;

	if (header->level == 0) // leaf node
	{
		for (i = 0; i < endian32(header->nrItems); i++)
		{
			item = (BtrfsItem *)nodePtr;

			switch (item->key.type)
			{
			case TYPE_DEV_ITEM:
				assert(endian32(item->size) == sizeof(BtrfsDevItem)); // ensure proper size
				assert(sizeof(BtrfsHeader) + endian32(item->offset) + endian32(item->size) <= endian32(super.nodeSize)); // ensure we're within bounds

				if (numDevices == -1)
				{
					devices = (BtrfsDevItem *)malloc(sizeof(BtrfsDevItem));
					numDevices = 0;
				}
				else
					devices = (BtrfsDevItem *)realloc(devices, sizeof(BtrfsDevItem) * (numDevices + 1));

				devices[numDevices] = *((BtrfsDevItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset)));

				numDevices++;
				break;
			case TYPE_CHUNK_ITEM:
				assert((endian32(item->size) - sizeof(BtrfsChunkItem)) % sizeof(BtrfsChunkItemStripe) == 0); // ensure proper 30+20n size
				assert(sizeof(BtrfsHeader) + endian32(item->offset) + endian32(item->size) <= endian32(super.nodeSize)); // ensure we're within bounds

				/* assuming that the chunks array is already allocated, as should be the case from the start */
				chunks = (Chunk *)realloc(chunks, sizeof(Chunk) * (numChunks + 1));

				chunks[numChunks].logiOffset = endian64(item->key.offset);
				chunks[numChunks].chunkItem = *((BtrfsChunkItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset)));
				chunks[numChunks].stripes = (BtrfsChunkItemStripe *)malloc(sizeof(BtrfsChunkItemStripe) *
					endian16(chunks[numChunks].chunkItem.numStripes));

				for (j = 0; j < endian16(chunks[numChunks].chunkItem.numStripes); j++)
					chunks[numChunks].stripes[j] = *((BtrfsChunkItemStripe *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset) +
						sizeof(BtrfsChunkItem) + (sizeof(BtrfsChunkItemStripe) * j)));

				numChunks++;
				break;
			default:
				printf("parseChunkTreeRec: found an item of unexpected type [0x%02x] in the tree!\n", item->key.type);
				break;
			}

			nodePtr += sizeof(BtrfsItem);
		}
	}
	else // non-leaf node
	{
		for (i = 0; i < endian32(header->nrItems); i++)
		{
			BtrfsKeyPtr *keyPtr = (BtrfsKeyPtr *)nodePtr;

			/* recurse down one level of the tree */
			parseChunkTreeRec(endian64(keyPtr->blockNum));

			nodePtr += sizeof(BtrfsKeyPtr);
		}
	}

	if (numDevices > 1)
		printf("parseChunkTreeRec: volumes with more than one device not yet supported!\n");

	free(nodeBlock);
}

void parseChunkTree()
{
	getSBChunks();
	
	parseChunkTreeRec(endian64(super.chunkTreeLAddr));
}

void parseRootTreeRec(unsigned __int64 addr)
{
	unsigned char *nodeBlock, *nodePtr;
	BtrfsHeader *header;
	BtrfsItem *item;
	int i, j;

	loadNode(addr, ADDR_LOGICAL, &nodeBlock, &header);

	assert(header->tree == OBJID_ROOT_TREE);
	
	nodePtr = nodeBlock + sizeof(BtrfsHeader);

	if (header->level == 0) // leaf node
	{
		for (i = 0; i < endian32(header->nrItems); i++)
		{
			item = (BtrfsItem *)nodePtr;

			switch (item->key.type)
			{
			case TYPE_ROOT_ITEM:
				assert(endian32(item->size) == sizeof(BtrfsRootItem)); // ensure proper size
				assert(sizeof(BtrfsHeader) + endian32(item->offset) + endian32(item->size) <= endian32(super.nodeSize)); // ensure we're within bounds

				if (numRoots == -1)
				{
					roots = (Root *)malloc(sizeof(Root));
					numRoots = 0;
				}
				else
					roots = (Root *)realloc(roots, sizeof(Root) * (numRoots + 1));

				roots[numRoots].objectID = (BtrfsObjID)endian64(item->key.objectID);
				roots[numRoots].rootItem = *((BtrfsRootItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset)));

				numRoots++;
				break;
			default:
				printf("parseRootTreeRec: found an item of unexpected type [0x%02x] in the tree!\n", item->key.type);
				break;
			}

			nodePtr += sizeof(BtrfsItem);
		}
	}
	else // non-leaf node
	{
		for (i = 0; i < endian32(header->nrItems); i++)
		{
			BtrfsKeyPtr *keyPtr = (BtrfsKeyPtr *)nodePtr;

			/* recurse down one level of the tree */
			parseRootTreeRec(endian64(keyPtr->blockNum));

			nodePtr += sizeof(BtrfsKeyPtr);
		}
	}

	free(nodeBlock);
}

void parseRootTree()
{
	parseRootTreeRec(endian64(super.rootTreeLAddr));
}

int parseFSTreeRec(unsigned __int64 addr, int operation, void *input1, void *input2, void *input3, void *output1, void *output2)
{
	unsigned char *nodeBlock, *nodePtr;
	BtrfsHeader *header;
	BtrfsItem *item;
	int i, j, doneEarly = 0, rtnCode;

	loadNode(addr, ADDR_LOGICAL, &nodeBlock, &header);

	assert(header->tree == OBJID_FS_TREE);
	
	nodePtr = nodeBlock + sizeof(BtrfsHeader);

	/* set default return code by operation */
	if (operation == FSOP_ID_TO_CHILD_IDS || operation == FSOP_DUMP_TREE)
		rtnCode = 0;
	else
		rtnCode = 1;

	if (operation == FSOP_DUMP_TREE)
	{
		printf("[Node] addr = 0x%016I64X level = 0x%02X nrItems = 0x%08X\n\n", addr, header->level, header->nrItems);

		if (header->level != 0)
		{
			for (i = 0; i < endian32(header->nrItems); i++)
			{
				BtrfsKeyPtr *keyPtr = (BtrfsKeyPtr *)(nodePtr + (sizeof(BtrfsKeyPtr) * i));

				printf("[%02X] {%016I64X|%016I64X} KeyPtr: block 0x%016I64X generation 0x%016I64X\n",
					i, keyPtr->key.objectID, keyPtr->key.offset, keyPtr->blockNum, keyPtr->generation);
			}
		}

		printf("\n");
	}

	if (header->level == 0) // leaf node
	{
		for (i = 0; i < endian32(header->nrItems); i++)
		{
			item = (BtrfsItem *)nodePtr;

			if (operation == FSOP_NAME_TO_ID)
			{
				/* pointer aliases */
				const BtrfsObjID *parentID = (const BtrfsObjID *)input1;
				const unsigned __int64 *hash = (const unsigned __int64 *)input2;
				const char *name = (const char *)input3;
				BtrfsObjID *childID = (BtrfsObjID *)output1;
				
				/* TODO: fix hash matching */
				if (item->key.type == TYPE_DIR_ITEM && endian64(item->key.objectID) == *parentID/* &&
					endian64(item->key.offset) == *hash*/)
				{
					BtrfsDirItem *dirItem = (BtrfsDirItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));

					while (1)
					{
						/* ensure that the variably sized item fits entirely in the node block */
						assert((unsigned char *)dirItem + sizeof(BtrfsDirItem) <= (unsigned char *)nodeBlock + endian32(super.nodeSize) &&
							(unsigned char *)dirItem + sizeof(BtrfsDirItem) + endian16(dirItem->m) + endian16(dirItem->n) <=
							(unsigned char *)nodeBlock + endian32(super.nodeSize));
					
						if (endian16(dirItem->n) == strlen(name) && memcmp((char *)dirItem + sizeof(BtrfsDirItem), name, endian16(dirItem->n)) == 0)
						{
							/* found a match */
							*childID = dirItem->child.objectID;

							rtnCode = 0;
							doneEarly = 1;
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
			else if (operation == FSOP_ID_TO_INODE)
			{
				/* pointer aliases */
				const BtrfsObjID *objectID = (const BtrfsObjID *)input1;
				BtrfsInodeItem *inode = (BtrfsInodeItem *)output1;

				if (item->key.type == TYPE_INODE_ITEM && endian64(item->key.objectID) == *objectID)
				{
					BtrfsInodeItem *inodeItem = (BtrfsInodeItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));
					
					/* ensure that the item fits entirely in the node block */
					assert((unsigned char *)inodeItem + sizeof(BtrfsInodeItem) <= (unsigned char *)nodeBlock + endian32(super.nodeSize));

					memcpy(inode, inodeItem, sizeof(BtrfsInodeItem));

					rtnCode = 0;
					doneEarly = 1;
				}
			}
			else if (operation == FSOP_ID_TO_CHILD_IDS)
			{
				/* pointer aliases */
				const BtrfsObjID *parentID = (const BtrfsObjID *)input1;
				unsigned __int64 *numChildren = (unsigned __int64 *)output1;
				BtrfsObjID **children = (BtrfsObjID **)output2;

				if (item->key.type == TYPE_DIR_ITEM && endian64(item->key.objectID) == *parentID)
				{
					BtrfsDirItem *dirItem = (BtrfsDirItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));
					
					while (1)
					{
						/* ensure that the variably sized item fits entirely in the node block */
						assert((unsigned char *)dirItem + sizeof(BtrfsDirItem) <= (unsigned char *)nodeBlock + endian32(super.nodeSize) &&
							(unsigned char *)dirItem + sizeof(BtrfsDirItem) + endian16(dirItem->m) + endian16(dirItem->n) <=
							(unsigned char *)nodeBlock + endian32(super.nodeSize));
						
						if (*children == NULL)
							*children = (BtrfsObjID *)malloc(sizeof(BtrfsObjID));
						else
							*children = (BtrfsObjID *)realloc(*children, sizeof(BtrfsObjID) * (*numChildren + 1));

						(*children)[*numChildren] = dirItem->child.objectID;

						(*numChildren)++;
						
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
			else if (operation == FSOP_ID_TO_NAME)
			{
				/* pointer aliases */
				const BtrfsObjID *childID = (const BtrfsObjID *)input1;
				char *name = (char *)output1;

				if (item->key.type == TYPE_DIR_ITEM)
				{
					BtrfsDirItem *dirItem = (BtrfsDirItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));
					
					while (1)
					{
						/* ensure that the variably sized item fits entirely in the node block */
						assert((unsigned char *)dirItem + sizeof(BtrfsDirItem) <= (unsigned char *)nodeBlock + endian32(super.nodeSize) &&
							(unsigned char *)dirItem + sizeof(BtrfsDirItem) + endian16(dirItem->m) + endian16(dirItem->n) <=
							(unsigned char *)nodeBlock + endian32(super.nodeSize));
						
						if (endian64(dirItem->child.objectID) == *childID)
						{
							memcpy(name, (char *)dirItem + sizeof(BtrfsDirItem), endian16(dirItem->n));
							name[endian16(dirItem->n)] = 0;

							doneEarly = 1;
							rtnCode = 0;
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
			else if (operation == FSOP_ID_TO_PARENT_ID)
			{
				/* pointer aliases */
				const BtrfsObjID *childID = (const BtrfsObjID *)input1;
				BtrfsObjID *parentID = (BtrfsObjID *)output1;

				if (item->key.type == TYPE_DIR_ITEM)
				{
					BtrfsDirItem *dirItem = (BtrfsDirItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));
					
					while (1)
					{
						/* ensure that the variably sized item fits entirely in the node block */
						assert((unsigned char *)dirItem + sizeof(BtrfsDirItem) <= (unsigned char *)nodeBlock + endian32(super.nodeSize) &&
							(unsigned char *)dirItem + sizeof(BtrfsDirItem) + endian16(dirItem->m) + endian16(dirItem->n) <=
							(unsigned char *)nodeBlock + endian32(super.nodeSize));
						
						if (endian64(dirItem->child.objectID) == *childID)
						{
							*parentID = item->key.objectID;

							doneEarly = 1;
							rtnCode = 0;
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
				printf("parseFSTreeRec: can't dump regular items\n");
			}
			else
				printf("parseFSTreeRec: unknown operation (0x%02x)!\n", operation);

			if (doneEarly)
				break;

			nodePtr += sizeof(BtrfsItem);
		}
	}
	else // non-leaf node
	{
		for (i = 0; i < endian32(header->nrItems); i++)
		{
			BtrfsKeyPtr *keyPtr = (BtrfsKeyPtr *)nodePtr;

			/* recurse down one level of the tree */
			if (parseFSTreeRec(endian64(keyPtr->blockNum), operation, input1, input2, input3, output1, output2) == 0)
				rtnCode = 0;

			nodePtr += sizeof(BtrfsKeyPtr);
		}
	}

	free(nodeBlock);

	return rtnCode;
}

int parseFSTree(int operation, void *input1, void *input2, void *input3, void *output1, void *output2)
{
	/* pre tasks */
	if (operation == FSOP_ID_TO_CHILD_IDS)
	{
		/* pointer aliases */
		const BtrfsObjID *parentID = (const BtrfsObjID *)input1;
		unsigned __int64 *numChildren = (unsigned __int64 *)output1;
		unsigned __int64 **children = (unsigned _int64 **)output2;

		*numChildren = 0;
		*children = NULL;
	}
	else if (operation == FSOP_DUMP_TREE)
		printf("parseFSTree: operation is FSOP_DUMP_TREE, preparing to take a dump...\n\n");
	
	return parseFSTreeRec(getFSRootBlockNum(), operation, input1, input2, input3, output1, output2);
}

unsigned __int64 getFSRootBlockNum()
{
	static int fsRoot = -1;
	int i;

	/* this only needs to be done the first time the function is called */
	if (fsRoot == -1)
	{
		/* no way we can find the FS root node without the root tree */
		assert(numRoots > 0 && roots != NULL);
	
		for (i = 0; i < numRoots; i++)
		{
			if (endian64(roots[i].objectID) == OBJID_FS_TREE)
			{
				fsRoot = i;
				break;
			}
		}

		/* can't fail */
		assert(fsRoot != -1);
	}

	return endian64(roots[fsRoot].rootItem.rootNodeBlockNum);
}

void dump()
{
	int i, j;

	/* take a dump */
	printf("dump: dumping devices\n\n");
	for (i = 0; i < numDevices; i++)
	{
		printf("devices[%d]:\n", i);
		printf("devID         0x%016X\n", endian64(devices[i].devID));
		printf("numBytes      0x%016X\n", endian64(devices[i].numBytes));
		printf("numBytesUsed  0x%016X\n", endian64(devices[i].numBytesUsed));
		printf("bestIOAlign           0x%08X\n", endian32(devices[i].bestIOAlign));
		printf("bestIOWidth           0x%08X\n", endian32(devices[i].bestIOWidth));
		printf("minIOSize             0x%08X\n", endian32(devices[i].minIOSize));
		printf("type          0x%016X\n", endian64(devices[i].type));
		printf("generation    0x%016X\n", endian64(devices[i].generation));
		printf("startOffset   0x%016X\n", endian64(devices[i].startOffset));
		printf("devGroup              0x%08X\n", endian32(devices[i].devGroup));
		printf("seekSpeed                   0x%02X\n", devices[i].seekSpeed);
		printf("bandwidth                   0x%02X\n", devices[i].bandwidth);
		printf("devUUID         "); for (j = 0; j < 16; j++) printf("%c", devices[i].devUUID[j]); printf("\n");
		printf("fsUUID          "); for (j = 0; j < 16; j++) printf("%c", devices[i].fsUUID[j]); printf("\n\n");
	}

	printf("dump: dumping chunks\n\n");
	for (i = 0; i < numChunks; i++)
	{
		printf("chunks[%d]: ", i);
		printf("size: 0x%016X; ", endian64(chunks[i].chunkItem.chunkSize));
		printf("0x%016X -> ", chunks[i].logiOffset);
		for (j = 0; j < endian16(chunks[i].chunkItem.numStripes); j++)
			printf("%s0x%016X", (j == 0 ? "" : ", "), endian64(chunks[i].stripes[j].offset));
		printf("\n");
	}
	printf("\n");

	printf("dump: dumping roots\n\n");
	for (i = 0; i < numRoots; i++)
	{
		printf("roots[%d]:\n", i);
		printf("[objectID]          0x%016X\n", roots[i].objectID);
		printf("inodeItem                          ...\n");
		printf("expectedGeneration  0x%016X\n", endian64(roots[i].rootItem.expectedGeneration));
		printf("objID               0x%016X\n", endian64(roots[i].rootItem.objID));
		printf("rootNodeBlockNum    0x%016X\n", endian64(roots[i].rootItem.rootNodeBlockNum));
		printf("byteLimit           0x%016X\n", endian64(roots[i].rootItem.byteLimit));
		printf("bytesUsed           0x%016X\n", endian64(roots[i].rootItem.bytesUsed));
		printf("lastGenSnapshot     0x%016X\n", endian64(roots[i].rootItem.lastGenSnapshot));
		printf("flags               0x%016X\n", endian64(roots[i].rootItem.flags));
		printf("numRefs                     0x%08X\n", endian32(roots[i].rootItem.numRefs));
		printf("dropProgress                       ...\n");
		printf("dropLevel                         0x%02X\n", roots[i].rootItem.dropLevel);
		printf("rootLevel                         0x%02X\n\n", roots[i].rootItem.rootLevel);
	}
}
