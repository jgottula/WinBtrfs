/* roottree_parser.cpp
 * root tree parser
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

#include "roottree_parser.h"
#include <cassert>
#include <vector>
#include "btrfs_system.h"
#include "endian.h"
#include "util.h"

extern BtrfsSuperblock super;
extern BtrfsObjID mountedSubvol;

std::vector<KeyedItem> rootTree;

void parseRootTreeRec(unsigned __int64 addr, RTOperation operation, void *input0, void *output0,
	int *returnCode, bool *shortCircuit)
{
	unsigned char *nodeBlock, *nodePtr;
	BtrfsHeader *header;
	BtrfsItem *item;
	KeyedItem kItem;

	nodeBlock = loadNode(addr, ADDR_LOGICAL, &header);

	assert(header->tree == OBJID_ROOT_TREE);
	
	nodePtr = nodeBlock + sizeof(BtrfsHeader);

	if (operation == RTOP_DUMP_TREE)
		printf("\n[Node] tree = 0x%I64x addr = 0x%I64x level = 0x%02x nrItems = 0x%08x\n", endian64(header->tree),
			addr, header->level, header->nrItems);

	if (header->level == 0) // leaf node
	{
		for (unsigned int i = 0; i < endian32(header->nrItems); i++)
		{
			item = (BtrfsItem *)nodePtr;

			if (operation == RTOP_LOAD)
			{
				switch (item->key.type)
				{
				case TYPE_INODE_ITEM:
					assert(endian32(item->size) == sizeof(BtrfsInodeItem)); // ensure proper size

					memcpy(&kItem.key, &item->key, sizeof(BtrfsDiskKey));
					kItem.data = malloc(endian32(item->size));
					memcpy(kItem.data, nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset), endian32(item->size));

					rootTree.push_back(kItem);
					break;
				case TYPE_INODE_REF:
					assert(endian32(item->size) >= sizeof(BtrfsInodeRef)); // ensure proper size range

					memcpy(&kItem.key, &item->key, sizeof(BtrfsDiskKey));
					kItem.data = malloc(endian32(item->size));
					memcpy(kItem.data, nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset), endian32(item->size));

					rootTree.push_back(kItem);
					break;
				case TYPE_DIR_ITEM:
					assert(endian32(item->size) >= sizeof(BtrfsDirItem)); // ensure proper size range

					memcpy(&kItem.key, &item->key, sizeof(BtrfsDiskKey));
					kItem.data = malloc(endian32(item->size));
					memcpy(kItem.data, nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset), endian32(item->size));

					rootTree.push_back(kItem);
					break;
				case TYPE_ROOT_ITEM:
					assert(endian32(item->size) == sizeof(BtrfsRootItem)); // ensure proper size

					memcpy(&kItem.key, &item->key, sizeof(BtrfsDiskKey));
					kItem.data = malloc(endian32(item->size));
					memcpy(kItem.data, nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset), endian32(item->size));

					rootTree.push_back(kItem);
					break;
				case TYPE_ROOT_BACKREF:
					assert(endian32(item->size) >= sizeof(BtrfsRootBackref)); // ensure proper size range

					memcpy(&kItem.key, &item->key, sizeof(BtrfsDiskKey));
					kItem.data = malloc(endian32(item->size));
					memcpy(kItem.data, nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset), endian32(item->size));

					rootTree.push_back(kItem);
					break;
				case TYPE_ROOT_REF:
					assert(endian32(item->size) >= sizeof(BtrfsRootRef)); // ensure proper size range

					memcpy(&kItem.key, &item->key, sizeof(BtrfsDiskKey));
					kItem.data = malloc(endian32(item->size));
					memcpy(kItem.data, nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset), endian32(item->size));

					rootTree.push_back(kItem);
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
					BtrfsDirItem *dirItem = (BtrfsDirItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset)),
						*firstDirItem = dirItem;

					while (true)
					{
						size_t len = endian16(dirItem->n);
						char *name = new char[len + 1];

						memcpy(name, dirItem->namePlusData, len);
						name[len] = 0;

						if (dirItem == firstDirItem)
							printf("  [%02x] ", i);
						else
							printf("       ");
						printf("DIR_ITEM parent: 0x%I64x hash: 0x%08I64x child: 0x%I64x -> '%s'\n",
							endian64(item->key.objectID), endian64(item->key.offset), endian64(dirItem->child.objectID), name);
						
						delete[] name;
						
						/* advance to the next DIR_ITEM if there are more */
						if (endian32(item->size) > ((char *)dirItem - (char *)firstDirItem) + sizeof(BtrfsDirItem) +
							endian16(dirItem->m) + endian16(dirItem->n))
							dirItem = (BtrfsDirItem *)((unsigned char *)dirItem + sizeof(BtrfsDirItem) +
								endian16(dirItem->m) + endian16(dirItem->n));
						else
							break;
					}

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
				{
					BtrfsRootBackref *rootBackref = (BtrfsRootBackref *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));
					size_t len = endian16(rootBackref->n);
					char *name = new char[len + 1];

					memcpy(name, rootBackref->name, len);
					name[len] = 0;
					
					printf("  [%02x] ROOT_BACKREF subtree: 0x%I64x -> '%s' tree: 0x%I64x\n", i,
						endian64(item->key.objectID), name, endian64(item->key.offset));

					delete[] name;
					break;
				}
				case TYPE_ROOT_REF:
				{
					BtrfsRootRef *rootRef = (BtrfsRootRef *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));
					size_t len = endian16(rootRef->n);
					char *name = new char[len + 1];

					memcpy(name, rootRef->name, len);
					name[len] = 0;
					
					printf("  [%02x] ROOT_REF tree: 0x%I64x subtree: 0x%I64x -> '%s'\n", i,
						endian64(item->key.objectID), endian64(item->key.offset), name);

					delete[] name;
					break;
				}
				default:
					printf("  [%02x] unknown {0x%I64x|0x%02x|0x%I64x}\n", i, endian64(item->key.objectID),
						item->key.type, endian64(item->key.offset));
					break;
				}
			}
			else if (operation == RTOP_DEFAULT_SUBVOL)
			{
				if (item->key.type == TYPE_DIR_ITEM && endian64(item->key.objectID) == OBJID_ROOT_TREE_DIR)
				{
					BtrfsDirItem *dirItem = (BtrfsDirItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));
					
					mountedSubvol = (BtrfsObjID)endian64(dirItem->child.objectID);

					*returnCode = 0;
					*shortCircuit = true;
				}
			}
			else if (operation == RTOP_GET_SUBVOL_ID)
			{
				const char *name = (const char *)input0;
				BtrfsObjID *subvolID = (BtrfsObjID *)output0;

				if (item->key.type == TYPE_ROOT_BACKREF)
				{
					BtrfsRootBackref *rootBackref = (BtrfsRootBackref *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));
					
					/* ideally, we would go back up the tree at this point and see if the chain of ROOT_REFs/ROOT_BACKREFs
						leads back to the FS tree; however, this is awkward, and currently, subtrees appear to only occur
						in the case of subvolumes, so it currently seems safe to assume that ANY subtree will be a valid subvolume.
						it's conceivable that in the future, other ROOT_REF'd subtrees might exist for other things,
						but for now, this solution seems fine */
					
					if (strlen(name) == endian16(rootBackref->n) && strncmp(name, rootBackref->name, endian16(rootBackref->n)) == 0)
					{
						*subvolID = (BtrfsObjID)endian64(item->key.objectID);

						*returnCode = 0;
						*shortCircuit = true;
					}
				}
			}
			else if (operation == RTOP_SUBVOL_EXISTS)
			{
				const BtrfsObjID *subvolID = (const BtrfsObjID *)input0;
				bool *exists = (bool *)output0;

				if (item->key.type == TYPE_ROOT_BACKREF && endian64(item->key.offset) == *subvolID)
				{
					*exists = true;

					*shortCircuit = true;
				}
			}
			else
				printf("parseRootTreeRec: unknown operation (0x%02x)!\n", operation);

			if (*shortCircuit)
				break;

			nodePtr += sizeof(BtrfsItem);
		}
	}
	else // non-leaf node
	{
		if (operation == RTOP_DUMP_TREE)
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
			parseRootTreeRec(endian64(keyPtr->blockNum), operation, input0, output0,
				returnCode, shortCircuit);

			if (*shortCircuit)
				break;

			nodePtr += sizeof(BtrfsKeyPtr);
		}
	}

	free(nodeBlock);
}

int parseRootTree(RTOperation operation, void *input0, void *output0)
{
	int returnCode;
	bool shortCircuit = false;
	
	switch (operation)
	{
	case RTOP_LOAD:				// always succeeds
	case RTOP_DUMP_TREE:		// always succeeds
	case RTOP_SUBVOL_EXISTS:	// always succeeds
		returnCode = 0;
		break;
	default:
		returnCode = 0x1;	// 1 bit = 1 part MUST be fulfilled
	}

	if (operation == RTOP_SUBVOL_EXISTS)
	{
		bool *exists = (bool *)output0;

		/* default to nonexistence */
		*exists = false;
	}
	
	parseRootTreeRec(endian64(super.rootTreeLAddr), operation, input0, output0,
		&returnCode, &shortCircuit);

	return returnCode;
}
