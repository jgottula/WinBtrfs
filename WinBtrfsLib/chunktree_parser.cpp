/* WinBtrfsLib/chunktree_parser.cpp
 * chunk tree parser
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

#include "chunktree_parser.h"
#include <cassert>
#include <vector>
#include "btrfs_system.h"
#include "endian.h"
#include "util.h"

extern std::vector<BtrfsSuperblock> supers;

std::vector<KeyedItem> chunkTree;

void parseChunkTreeRec(LogiAddr addr, CTOperation operation)
{
	unsigned char *nodeBlock, *nodePtr;
	BtrfsHeader *header;
	BtrfsItem *item;
	KeyedItem kItem;
	unsigned short *temp;
	
	nodeBlock = loadNode(addr, &header);

	assert(header->tree == OBJID_CHUNK_TREE);
	
	nodePtr = nodeBlock + sizeof(BtrfsHeader);

	if (operation == CTOP_DUMP_TREE)
		printf("\n[Node] tree = 0x%I64x addr = 0x%I64x level = 0x%02x nrItems = 0x%08x\n", endian64(header->tree),
			addr, header->level, header->nrItems);

	if (header->level == 0) // leaf node
	{
		for (unsigned int i = 0; i < endian32(header->nrItems); i++)
		{
			item = (BtrfsItem *)nodePtr;

			if (operation == CTOP_LOAD)
			{
				switch (item->key.type)
				{
				case TYPE_DEV_ITEM:
					assert(endian32(item->size) == sizeof(BtrfsDevItem)); // ensure proper size

					memcpy(&kItem.key, &item->key, sizeof(BtrfsDiskKey));
					kItem.data = malloc(endian32(item->size));
					memcpy(kItem.data, nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset), endian32(item->size));

					chunkTree.push_back(kItem);
					break;
				case TYPE_CHUNK_ITEM:
					assert((endian32(item->size) - sizeof(BtrfsChunkItem)) % sizeof(BtrfsChunkItemStripe) == 0); // ensure proper 30+20n size

					temp = (unsigned short *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset) + 0x2c);

					/* check the ACTUAL size now that we have it */
					assert(endian32(item->size) == sizeof(BtrfsChunkItem) + (*temp * sizeof(BtrfsChunkItemStripe)));

					memcpy(&kItem.key, &item->key, sizeof(BtrfsDiskKey));
					kItem.data = malloc(endian32(item->size));
					memcpy(kItem.data, nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset), endian32(item->size));

					chunkTree.push_back(kItem);
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
					printf("  [%02x] DEV_ITEM devID: 0x%I64x devUUID: %s\n"
						"                devGroup: 0x%x offset: 0x%I64x size: 0x%I64x\n", i,
						endian64(item->key.offset), uuid, endian32(devItem->devGroup),
						endian64(devItem->startOffset), endian64(devItem->numBytes));
					break;
				}
				case TYPE_CHUNK_ITEM:
				{
					BtrfsChunkItem *chunkItem = (BtrfsChunkItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));
					char type[32];
					
					bgFlagsToStr((BlockGroupFlags)endian64(chunkItem->type), type);
					printf("  [%02x] CHUNK_ITEM size: 0x%I64x logi: 0x%I64x type: %s\n", i, endian64(chunkItem->chunkSize),
						endian64(item->key.offset), type);
					for (int j = 0; j < chunkItem->numStripes; j++)
						printf("         + STRIPE devID: 0x%I64x offset: 0x%I64x\n", endian64(chunkItem->stripes[j].devID),
							endian64(chunkItem->stripes[j].offset));
					break;
				}
				default:
					printf("  [%02x] unknown {0x%I64x|0x%02x|0x%I64x}\n", i, endian64(item->key.objectID),
						item->key.type, endian64(item->key.offset));
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
			for (unsigned int i = 0; i < endian32(header->nrItems); i++)
			{
				BtrfsKeyPtr *keyPtr = (BtrfsKeyPtr *)(nodePtr + (sizeof(BtrfsKeyPtr) * i));

				printf("  [%02x] {%I64x|%I64x} KeyPtr: block 0x%016I64x generation 0x%016I64x\n",
					i, endian64(keyPtr->key.objectID), endian64(keyPtr->key.offset),
					endian64(keyPtr->blockNum), endian64(keyPtr->generation));
			}
		}

		for (unsigned int i = 0; i < endian32(header->nrItems); i++)
		{
			BtrfsKeyPtr *keyPtr = (BtrfsKeyPtr *)nodePtr;

			/* recurse down one level of the tree */
			parseChunkTreeRec(endian64(keyPtr->blockNum), operation);

			nodePtr += sizeof(BtrfsKeyPtr);
		}
	}

	free(nodeBlock);
}

void parseChunkTree(CTOperation operation)
{
	parseChunkTreeRec(endian64(supers[0].ctRoot), operation);
}
