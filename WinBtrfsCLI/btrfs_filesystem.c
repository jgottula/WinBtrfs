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

#include <stdio.h>
#include <assert.h>
#include <Windows.h>
#include "structures.h"
#include "constants.h"
#include "endian.h"
#include "crc32c.h"
#include "btrfs_filesystem.h"

extern WCHAR devicePath[MAX_PATH];

HANDLE hRead = INVALID_HANDLE_VALUE, hReadMutex = INVALID_HANDLE_VALUE;
BtrfsSuperblock super;
BtrfsDevItem *devices = NULL;
int numDevices = -1;
Chunk *chunks = NULL;
int numChunks = -1;
Root *roots = NULL;
int numRoots = -1;

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

DWORD readBlock(unsigned __int64 addr, int addrType, unsigned __int64 len, unsigned char *dest)
{
	LARGE_INTEGER li;
	DWORD bytesRead;

	/* hope you called init first! */
	assert(hRead != INVALID_HANDLE_VALUE);
	assert(hReadMutex != INVALID_HANDLE_VALUE);

	if (addrType == ADDR_LOGICAL)
		addr = logiToPhys(addr, len);

	/* because Win32 uses a signed (??) value for the address from which to read,
		our address space is cut in half from what Btrfs technically allows */
	li.QuadPart = (LONGLONG)addr;

	/* using SetFilePointerEx and then ReadFile is clearly not threadsafe, so for now
		I'm using a mutex here; in the future, this should be fully threaded
		(perhaps a separate read handle per thread? this would require a CreateFile on each
		call—or perhaps declare a static local that would retain value per thread?) */
	if (WaitForSingleObject(hReadMutex, 10000) != WAIT_OBJECT_0)
		goto error;

	if (SetFilePointerEx(hRead, li, NULL, 0) == 0)
		goto error;

	if (ReadFile(hRead, dest, (DWORD)len, &bytesRead, NULL) == 0 || bytesRead != len)
		goto error;

	ReleaseMutex(hReadMutex);
	return 0;

error:
	ReleaseMutex(hReadMutex);
	return GetLastError();
}

DWORD readPrimarySB()
{
	return readBlock(SUPERBLOCK_1_PADDR, ADDR_PHYSICAL, sizeof(BtrfsSuperblock), (unsigned char *)&super);
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

	if (readBlock(SUPERBLOCK_2_PADDR, ADDR_PHYSICAL, sizeof(BtrfsSuperblock), (unsigned char *)&s2) == 0 &&
		validateSB(&s2) == 0 && endian64(s2.generation) > bestGen)
		best = 2, sBest = &s2, bestGen = endian64(s2.generation);

	if (readBlock(SUPERBLOCK_2_PADDR, ADDR_PHYSICAL, sizeof(BtrfsSuperblock), (unsigned char *)&s3) == 0 &&
		validateSB(&s3) == 0 && endian64(s3.generation) > bestGen)
		best = 3, sBest = &s3, bestGen = endian64(s3.generation);

	if (readBlock(SUPERBLOCK_2_PADDR, ADDR_PHYSICAL, sizeof(BtrfsSuperblock), (unsigned char *)&s4) == 0 &&
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
	assert(readBlock(blockAddr, addrType, blockSize, *nodeDest) == 0);

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

				roots[numRoots].objectID = endian64(item->key.objectID);
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

/* extend this function to have an option for the operation we want, plus an output/returnval */
void parseFSTreeRec(unsigned __int64 addr, int operation, void *inputA, void *inputB, void *inputC, void *output)
{
	unsigned char *nodeBlock, *nodePtr;
	BtrfsHeader *header;
	BtrfsItem *item;
	int i, j, done = 0;

	loadNode(addr, ADDR_LOGICAL, &nodeBlock, &header);

	assert(header->tree == OBJID_FS_TREE);
	
	nodePtr = nodeBlock + sizeof(BtrfsHeader);

	if (header->level == 0) // leaf node
	{
		for (i = 0; i < endian32(header->nrItems); i++)
		{
			item = (BtrfsItem *)nodePtr;

			if (operation == FSOP_NAME_TO_ID)
			{
				/* pointer aliases */
				const unsigned __int64 *parentID = (const unsigned __int64 *)inputA;
				const unsigned __int64 *hash = (const unsigned __int64 *)inputB;
				const char *name = (const char *)inputC;
				unsigned __int64 *childID = (unsigned __int64 *)output;
				
				printf("parseFSTreeRec: name hashing appears to be broken, please fix me!\n");
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
							done = 1;
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
				const unsigned __int64 *objectID = (const unsigned __int64 *)inputA;
				BtrfsInodeItem *inode = (BtrfsInodeItem *)output;

				if (item->key.type == TYPE_INODE_ITEM && endian64(item->key.objectID) == *objectID)
				{
					BtrfsInodeItem *inodeItem = (BtrfsInodeItem *)(nodeBlock + sizeof(BtrfsHeader) + endian32(item->offset));
					
					/* ensure that the item fits entirely in the node block */
					assert((unsigned char *)inodeItem + sizeof(BtrfsInodeItem) <= (unsigned char *)nodeBlock + endian32(super.nodeSize));

					memcpy(inode, inodeItem, sizeof(BtrfsInodeItem));

					done = 1;
				}
			}
			else
				printf("parseFSTreeRec: unknown operation (0x%02x)!\n", operation);

#if 0
			switch (item->key.type)
			{
			case TYPE_INODE_ITEM:
				break;
			case TYPE_INODE_REF:
				break;
			case TYPE_DIR_ITEM:
				/* name & data extend past end of the struct */
				/* may repeat */
				break;
			case TYPE_DIR_INDEX:
				/* name & data extend past end of the struct */
				break;
			case TYPE_EXTENT_DATA:
				/* either inline data or a BtrfsExtentDataNonInline struct will follow */
				break;
			default:
				printf("parseFSTreeRec: found an item of unexpected type [0x%02x] in the tree!\n", item->key.type);
				break;
			}
#endif

			if (done)
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
			parseFSTreeRec(endian64(keyPtr->blockNum), operation, inputA, inputB, inputC, output);

			nodePtr += sizeof(BtrfsKeyPtr);
		}
	}

	free(nodeBlock);
}

void parseFSTree(int operation, void *inputA, void *inputB, void *inputC, void *output)
{
	parseFSTreeRec(getFSRootBlockNum(), operation, inputA, inputB, inputC, output);
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

void validatePath(char *path)
{
	int i;

	for (i = 0; i < strlen(path); i++)
	{
		assert(i != 0 || path[i] == '\\'); // MUST start with a backslash
		assert(i == 0 || i != strlen(path) - 1 || path[i] != '\\'); // CANNOT end with a backslash (if len > 1)

		if (path[i] == '\\' && i > 0)
			assert(path[i - 1] != '\\'); // no double backslashes within paths
	}
}

/* path MUST be validated for this to work properly */
int componentizePath(char *path, char ***output)
{
	size_t len = strlen(path), compLen = 0, i;
	int numComponents = 0, compIdx = 0, compC = 0;

	if (len == 1 && path[0] == '\\')
		return 0;

	for (i = 0; i < len; i++)
	{
		if (path[i] == '\\')
			numComponents++;
	}

	/* allocate the array of pointers */
	*output = (char **)malloc(sizeof(char *) * numComponents);

	for (i = 1; i < len; i++) // skip the first backslash
	{
		if (path[i] != '\\')
			compLen++;
		else
		{
			/* allocate this individual pointer in the array */
			(*output)[compIdx++] = (char *)malloc(compLen + 1);
			compLen = 0;
		}
	}

	/* get the last one */
	(*output)[numComponents - 1] = (char *)malloc(compLen + 1);

	compIdx = 0;

	for (i = 1; i < len; i++) // again, skip the first backslash
	{
		if (path[i] != '\\')
		{
			/* fill in the component name, char by char */
			(*output)[compIdx][compC++] = path[i];
		}
		else
		{
			/* null terminate */
			(*output)[compIdx++][compC] = 0;
			compC = 0;
		}
	}

	/* get the last one */
	(*output)[numComponents - 1][compC] = 0;

	return numComponents;
}

void getInode(char *path, BtrfsInodeItem *inode)
{
	char **components;
	unsigned __int64 parentID = OBJID_ROOT_DIR, childID, hash;
	int i, numComponents = -1;

	validatePath(path);
	numComponents = componentizePath(path, &components);

	for (i = 0; i < numComponents; i++)
	{
		childID = -1;
		hash = crc32c(0, (const unsigned char *)(components[i]), strlen(components[i]));
		parseFSTree(FSOP_NAME_TO_ID, &parentID, &hash, components[i], &childID);

		/* can't fail */
		assert(childID != -1);

		parentID = childID;
	}

	parseFSTree(FSOP_ID_TO_INODE, &parentID, NULL, NULL, inode);
}

void test()
{
	BtrfsInodeItem inode;

	getInode("\\dirA\\dir1\\smiley.txt", &inode);
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
