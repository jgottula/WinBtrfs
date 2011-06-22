/* fstree_parser.cpp
 * fs tree parser
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

#include <cstdio>
#include <cassert>
#include <vector>
#include "endian.h"
#include "util.h"
#include "btrfs_system.h"
#include "fstree_parser.h"

extern BtrfsSuperblock super;
extern BtrfsObjID defaultSubvol;

void parseFSTreeRec(unsigned __int64 addr, FSOperation operation, void *input1, void *input2, void *input3,
	void *output1, void *output2, void *temp, int *returnCode, bool *shortCircuit)
{
	unsigned char *nodeBlock, *nodePtr;
	BtrfsHeader *header;
	BtrfsItem *item;

	nodeBlock = loadNode(addr, ADDR_LOGICAL, &header);
	
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
				const unsigned int *hash = (const unsigned int *)input2;
				const char *name = (const char *)input3;
				BtrfsObjID *childID = (BtrfsObjID *)output1;
				
				if (item->key.type == TYPE_DIR_ITEM && endian64(item->key.objectID) == *parentID &&
					(unsigned int)(endian64(item->key.offset)) == *hash)
				{
					BtrfsDirItem *dirItem = (BtrfsDirItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset)),
						*firstDirItem = dirItem;

					while (true)
					{
						if (endian16(dirItem->n) == strlen(name) && memcmp(dirItem->namePlusData, name, endian16(dirItem->n)) == 0)
						{
							/* found a match */
							*childID = dirItem->child.objectID;

							*returnCode = 0;
							*shortCircuit = true;
							break;
						}
						
						/* advance to the next DIR_ITEM if there are more */
						if (endian32(item->size) > ((char *)dirItem - (char *)firstDirItem) + sizeof(BtrfsDirItem) +
							endian16(dirItem->m) + endian16(dirItem->n))
							dirItem = (BtrfsDirItem *)((unsigned char *)dirItem + sizeof(BtrfsDirItem) +
								endian16(dirItem->m) + endian16(dirItem->n));
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
				case TYPE_XATTR_ITEM:
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
						printf("XATTR_ITEM 0x%I64x hash: 0x%08I64x name: '%s'\n", endian64(item->key.objectID),
							endian64(item->key.offset), name);
						
						delete[] name;
						
						/* advance to the next XATTR_ITEM if there are more */
						if (endian32(item->size) > ((char *)dirItem - (char *)firstDirItem) + sizeof(BtrfsDirItem) +
							endian16(dirItem->m) + endian16(dirItem->n))
							dirItem = (BtrfsDirItem *)((unsigned char *)dirItem + sizeof(BtrfsDirItem) +
								endian16(dirItem->m) + endian16(dirItem->n));
						else
							break;
					}
					
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
						{
							dirItem = (BtrfsDirItem *)((unsigned char *)dirItem + sizeof(BtrfsDirItem) +
								endian16(dirItem->m) + endian16(dirItem->n));
						}
						else
							break;
					}
					
					break;
				}
				case TYPE_DIR_INDEX:
					printf("  [%02x] DIR_INDEX 0x%I64x = idx 0x%I64x\n", i, endian64(item->key.objectID),
						endian64(item->key.offset));
					break;
				case TYPE_EXTENT_DATA:
				{
					BtrfsExtentData *extentData = (BtrfsExtentData *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));
					static const char fdTypeStrs[4][9] = { "inline", "regular", "prealloc", "unknown" };

					printf("  [%02x] EXTENT_DATA 0x%I64x offset: 0x%I64x size: 0x%I64x type: %s\n", i,
						endian64(item->key.objectID), endian64(item->key.offset), endian64(extentData->n),
						fdTypeStrs[extentData->type]);
					if (extentData->type != FILEDATA_INLINE)
					{
						BtrfsExtentDataNonInline *nonInlinePart =(BtrfsExtentDataNonInline *)(nodeBlock +
							sizeof(BtrfsHeader) + endian32(item->offset) + sizeof(BtrfsExtentData));

						printf("                   addr: 0x%I64x size: 0x%I64x offset: 0x%I64x\n",
							endian64(nonInlinePart->extAddr), endian64(nonInlinePart->extSize),
							endian64(nonInlinePart->offset));
					}
					break;
				}
				default:
					printf("  [%02x] unknown {0x%I64x|0x%02x|0x%I64x}\n", i, endian64(item->key.objectID),
						item->key.type, endian64(item->key.offset));
					break;
				}
			}
			else if (operation == FSOP_GET_FILE_PKG)
			{
				const BtrfsObjID *objectID = (const BtrfsObjID *)input1;
				FilePkg *filePkg = (FilePkg *)output1;
				
				/* it's safe to jump out once we pass the object ID in question */
				if (item->key.objectID > *objectID)
				{
					*shortCircuit = true;
					break;
				}

				if (item->key.type == TYPE_INODE_ITEM && endian64(item->key.objectID) == *objectID) // inode
				{
					BtrfsInodeItem *inodeItem = (BtrfsInodeItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));
					
					memcpy(&(filePkg->inode), inodeItem, sizeof(BtrfsInodeItem));
					
					*returnCode &= ~0x1; // clear bit 0

					if (inodeItem->stMode & S_IFDIR)
						*returnCode &= ~0x4; // clear bit 2; we don't need extent info for dirs
				}
				else if (item->key.type == TYPE_DIR_ITEM) // name & parent
				{
					BtrfsDirItem *dirItem = (BtrfsDirItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset)),
						*firstDirItem = dirItem;
					
					while (true)
					{
						if (endian64(dirItem->child.objectID) == *objectID) // parent info
						{
							memcpy(filePkg->name, dirItem->namePlusData,
								(endian16(dirItem->n) <= 255 ? endian16(dirItem->n) : 255)); // limit to 255
							filePkg->name[endian16(dirItem->n)] = 0;

							filePkg->parentID = (BtrfsObjID)endian64(item->key.objectID);
							
							*returnCode &= ~0x2; // clear bit 1
						}
						
						/* advance to the next DIR_ITEM if there are more */
						if (endian32(item->size) > ((char *)dirItem - (char *)firstDirItem) + sizeof(BtrfsDirItem) +
							endian16(dirItem->m) + endian16(dirItem->n))
						{
							dirItem = (BtrfsDirItem *)((unsigned char *)dirItem + sizeof(BtrfsDirItem) +
								endian16(dirItem->m) + endian16(dirItem->n));
						}
						else
							break;
					}
				}
				else if (item->key.type == TYPE_EXTENT_DATA && item->key.objectID == *objectID) // extent information
				{
					BtrfsExtentData *extentData = (BtrfsExtentData *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));

					filePkg->extents = (KeyedItem *)realloc(filePkg->extents,
						(filePkg->numExtents + 1) * sizeof(KeyedItem));
					filePkg->extents[filePkg->numExtents].data = (BtrfsExtentData *)malloc(endian32(item->size));

					memcpy(&(filePkg->extents[filePkg->numExtents].key), &item->key, sizeof(BtrfsDiskKey));
					memcpy(filePkg->extents[filePkg->numExtents].data, extentData, endian32(item->size));

					filePkg->numExtents++;
				}
			}
			else if (operation == FSOP_DIR_LIST)
			{
				const BtrfsObjID *objectID = (const BtrfsObjID *)input1;
				DirList *dirList = (DirList *)output1;
				
				if (item->key.type == TYPE_INODE_ITEM) // inode
				{
					BtrfsInodeItem *inodeItem = (BtrfsInodeItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));
					
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
							
							/* don't break out of the loop: there may be multiple entries that need
								the same inode loaded in (hard links, for example) */
						}
					}
				}
				else if (item->key.type == TYPE_DIR_ITEM) // allocate, ID, parent, name
				{
					BtrfsDirItem *dirItem = (BtrfsDirItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset)),
						*firstDirItem = dirItem;
					
					while (true)
					{
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
						if (endian32(item->size) > ((char *)dirItem - (char *)firstDirItem) + sizeof(BtrfsDirItem) +
							endian16(dirItem->m) + endian16(dirItem->n))
							dirItem = (BtrfsDirItem *)((unsigned char *)dirItem + sizeof(BtrfsDirItem) +
								endian16(dirItem->m) + endian16(dirItem->n));
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

	free(nodeBlock);
}

int parseFSTree(BtrfsObjID tree, FSOperation operation, void *input1, void *input2, void *input3, void *output1, void *output2)
{
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

		filePkg->numExtents = 0;
		filePkg->extents = (KeyedItem *)malloc(0);

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

	parseFSTreeRec(getTreeRootAddr(tree), operation, input1, input2, input3, output1, output2,
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
