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

#include "fstree_parser.h"
#include <cassert>
#include <cstdio>
#include "btrfs_system.h"
#include "endian.h"
#include "util.h"

void parseFSTreeRec(unsigned __int64 addr, BtrfsObjID tree, FSOperation operation, void *input0, void *input1, void *input2,
	void *output0, void *output1, int *returnCode, bool *shortCircuit)
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
		for (unsigned int i = 0; i < endian32(header->nrItems); i++)
		{
			item = (BtrfsItem *)nodePtr;

			if (operation == FSOP_NAME_TO_ID)
			{
				const BtrfsObjID *parentID = (const BtrfsObjID *)input0;
				const unsigned int *hash = (const unsigned int *)input1;
				const char *name = (const char *)input2;
				BtrfsObjID *childID = (BtrfsObjID *)output0;
				bool *isSubvolume = (bool *)output1;
				
				if (item->key.type == TYPE_DIR_ITEM && endian64(item->key.objectID) == *parentID &&
					(unsigned int)(endian64(item->key.offset)) == *hash)
				{
					BtrfsDirItem *dirItem = (BtrfsDirItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset)),
						*firstDirItem = dirItem;

					while (true)
					{
						if (endian16(dirItem->n) == strlen(name) && strncmp(dirItem->namePlusData, name, endian16(dirItem->n)) == 0)
						{
							/* these are the only EXPECTED child types; others shouldn't probably appear */
							assert(dirItem->child.type == TYPE_INODE_ITEM || dirItem->child.type == TYPE_ROOT_ITEM);
							
							/* found a match */
							*childID = dirItem->child.objectID;
							*isSubvolume = (dirItem->child.type == TYPE_ROOT_ITEM);

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
					static const char childTypeStrs[9][10] = { "unknown", "file", "directory", "char", "block",
						"FIFO", "socket", "symlink", "xattr" };

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
						printf("XATTR_ITEM 0x%I64x -> '%s' hash: 0x%08I64x\n"
							"                type: %s data: 0x%x bytes\n",
							endian64(item->key.objectID), name, endian64(item->key.offset),
							childTypeStrs[dirItem->childType], endian16(dirItem->m));
						
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
					static const char childTypeStrs[9][10] = { "unknown", "file", "directory", "char", "block",
						"FIFO", "socket", "symlink", "xattr" };

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
						printf("DIR_ITEM parent: 0x%I64x hash: 0x%08I64x\n"
							"                child: 0x%I64x -> '%s' type: %s%s\n",
							endian64(item->key.objectID), endian64(item->key.offset),
							endian64(dirItem->child.objectID), name, childTypeStrs[dirItem->childType],
							(dirItem->child.type == TYPE_ROOT_ITEM ? " (subvolume)" : ""));
						
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
				const BtrfsObjID *objectID = (const BtrfsObjID *)input0;
				FilePkg *filePkg = (FilePkg *)output0;
				
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
				else if (item->key.type == TYPE_DIR_ITEM) // name
				{
					BtrfsDirItem *dirItem = (BtrfsDirItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset)),
						*firstDirItem = dirItem;
					
					while (true)
					{
						if (endian64(dirItem->child.objectID) == *objectID)
						{
							memcpy(filePkg->name, dirItem->namePlusData,
								(endian16(dirItem->n) <= 255 ? endian16(dirItem->n) : 255)); // limit to 255
							filePkg->name[endian16(dirItem->n)] = 0;
							
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
				const FilePkg *filePkg = (const FilePkg *)input0;
				const bool *root = (const bool *)input1;
				DirList *dirList = (DirList *)output0;
				
				if (item->key.type == TYPE_INODE_ITEM) // inode
				{
					BtrfsInodeItem *inodeItem = (BtrfsInodeItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));

					for (int j = (*root ? 0 : 2); j < dirList->numEntries; j++) // try to find a matching entry, skip '.' and '..'
					{
						if (tree == dirList->entries[j].fileID.treeID &&
							endian64(item->key.objectID) == dirList->entries[j].fileID.objectID)
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
						if (endian64(item->key.objectID) == filePkg->fileID.objectID)
						{
							if (dirList->entries == NULL)
								dirList->entries = (FilePkg *)malloc(sizeof(FilePkg));
							else
								dirList->entries = (FilePkg *)realloc(dirList->entries, sizeof(FilePkg) * (dirList->numEntries + 1));

							assert(dirItem->child.type == TYPE_INODE_ITEM || dirItem->child.type == TYPE_ROOT_ITEM);

							if (dirItem->child.type == TYPE_INODE_ITEM)
							{
								dirList->entries[dirList->numEntries].fileID.treeID = tree;
								dirList->entries[dirList->numEntries].fileID.objectID = (BtrfsObjID)endian64(dirItem->child.objectID);
							}
							else
							{
								dirList->entries[dirList->numEntries].fileID.treeID = (BtrfsObjID)endian64(dirItem->child.objectID);
								dirList->entries[dirList->numEntries].fileID.objectID = OBJID_ROOT_DIR;
							}

							dirList->entries[dirList->numEntries].parentID.treeID = tree;
							dirList->entries[dirList->numEntries].parentID.objectID = (BtrfsObjID)endian64(item->key.objectID);

							memcpy(dirList->entries[dirList->numEntries].name, dirItem->namePlusData,
								(endian16(dirItem->n) <= 255 ? endian16(dirItem->n) : 255)); // limit to 255
							dirList->entries[dirList->numEntries].name[endian16(dirItem->n)] = 0;

							dirList->numEntries++;
							
							(*returnCode)++;
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
			else if (operation == FSOP_GET_INODE)
			{
				const BtrfsObjID *objectID = (const BtrfsObjID *)input0;
				BtrfsInodeItem *inode = (BtrfsInodeItem *)output0;
				
				/* abort once we pass the object ID in question */
				if (item->key.objectID > *objectID)
				{
					*shortCircuit = true;
					break;
				}

				if (item->key.type == TYPE_INODE_ITEM && endian64(item->key.objectID) == *objectID) // inode
				{
					BtrfsInodeItem *inodeItem = (BtrfsInodeItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));
					
					memcpy(inode, inodeItem, sizeof(BtrfsInodeItem));
					
					*returnCode = 0;
					*shortCircuit = true;
					break;
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
			parseFSTreeRec(endian64(keyPtr->blockNum), tree, operation, input0, input1, input2,
				output0, output1, returnCode, shortCircuit);

			if (*shortCircuit)
				break;

			nodePtr += sizeof(BtrfsKeyPtr);
		}
	}

	free(nodeBlock);
}

int parseFSTree(BtrfsObjID tree, FSOperation operation, void *input0, void *input1, void *input2, void *output0, void *output1)
{
	int returnCode;
	bool shortCircuit = false;
	
	switch (operation)
	{
	case FSOP_DUMP_TREE:		// always succeeds
	case FSOP_DIR_LIST:			// begins at zero for other reasons
		returnCode = 0;
		break;
	case FSOP_GET_FILE_PKG:
		returnCode = 0x1; // always need the inode
		if (*((BtrfsObjID *)input0) != OBJID_ROOT_DIR)
			returnCode |= 0x2; // needs name for all except the root dir
		break;
	default:
		returnCode = 0x1; // 1 bit = 1 part MUST be fulfilled
	}
	
	/* pre tasks */
	if (operation == FSOP_GET_FILE_PKG)
	{
		const BtrfsObjID *objectID = (const BtrfsObjID *)input0;
		FilePkg *filePkg = (FilePkg *)output0;

		filePkg->fileID.treeID = tree;
		filePkg->fileID.objectID = *objectID;

		filePkg->numExtents = 0;
		filePkg->extents = (KeyedItem *)malloc(0);

		/* for the special case of the root dir, this stuff wouldn't get filled in by any other means */
		if (*objectID == OBJID_ROOT_DIR)
		{
			strcpy(filePkg->name, "ROOT_DIR");
			memset(&filePkg->parentID, 0, sizeof(FileID));
		}
	}
	else if (operation == FSOP_DIR_LIST)
	{
		const FilePkg *filePkg = (const FilePkg *)input0;
		const bool *root = (const bool *)input1;
		DirList *dirList = (DirList *)output0;

		if (!(*root))
		{
			dirList->numEntries = 2;
			dirList->entries = (FilePkg *)malloc(2 * sizeof(FilePkg));

			/* add '.' to the list */
			memcpy(&dirList->entries[0], filePkg, sizeof(FilePkg));
			strcpy(dirList->entries[0].name, ".");

			/* add '..' to the list */
			memcpy(&dirList->entries[1].fileID, &filePkg->parentID, sizeof(FileID));
			memcpy(&dirList->entries[1].inode, &filePkg->parentInode, sizeof(BtrfsInodeItem));
			strcpy(dirList->entries[1].name, "..");
			dirList->entries[1].hidden = false;
		}
		else
		{
			dirList->numEntries = 0;
			dirList->entries = NULL;
		}
	}

	parseFSTreeRec(getTreeRootAddr(tree), tree, operation, input0, input1, input2, output0, output1,
		&returnCode, &shortCircuit);

	if (operation == FSOP_GET_FILE_PKG)
	{
		FilePkg *filePkg = (FilePkg *)output0;

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
		const bool *root = (const bool *)input1;
		DirList *dirList = (DirList *)output0;

		for (size_t i = (*root ? 0 : 2); i < dirList->numEntries; i++) // skip '.' and '..'
		{
			if (dirList->entries[i].fileID.treeID != tree)
			{
				BtrfsObjID rootID = OBJID_ROOT_DIR;
				
				parseFSTree(dirList->entries[i].fileID.treeID, FSOP_GET_INODE, &rootID, NULL, NULL,
					&dirList->entries[i].inode, NULL);
				returnCode--;
			}
		}

		if (returnCode == 0)
		{
			/* skip the first two items (. and ..) if this is the volume root */
			for (size_t i = (*root ? 0 : 2); i < dirList->numEntries; i++)
			{
				if (dirList->entries[i].name[0] == '.')
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
