/* btrfs_operations.c
 * high-level filesystem operations
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
#include "constants.h"
#include "structures.h"
#include "crc32c.h"
#include "endian.h"
#include "btrfs_system.h"

void validatePath(const char *input, char *output)
{
	size_t i, c = 0, len = strlen(input);

	/* convert "" into "\\" */
	if (len == 0)
	{
		output[0] = '\\';
		output[1] = 0;
		return;
	}

	/* ensure that it begins with a backslash */
	if (input[0] != '\\')
		output[c++] = '\\';

	for (i = 0; i < len; i++)
	{
		/* don't copy over double/triple/n-tuple backslashes */
		if (i == 0 || input[i] != '\\' || input[i - 1] != '\\')
			output[c++] = input[i];
	}

	/* ensure that it does NOT end with a backslash */
	if (output[c - 1] == '\\')
		c--;

	output[c] = 0;
}

/* path MUST be validated for this to work properly */
int componentizePath(const char *path, char ***output)
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

int getPathID(const char *path, unsigned __int64 *output)
{
	char vPath[MAX_PATH], **components;
	unsigned __int64 parentID = OBJID_ROOT_DIR, childID, hash;
	int i, numComponents = -1;

	validatePath(path, vPath);
	numComponents = componentizePath(path, &components);

	for (i = 0; i < numComponents; i++)
	{
		childID = -1;
		hash = crc32c(0, (const unsigned char *)(components[i]), strlen(components[i]));

		if (parseFSTree(FSOP_NAME_TO_ID, &parentID, &hash, components[i], &childID, NULL) != 0)
			return 1;

		parentID = childID;
	}

	*output = parentID;

	return 0;
}

int getInode(unsigned __int64 objectID, Inode *output, int checkHidden)
{
	BtrfsInodeItem inodeItem;
	char name[MAX_PATH];

	if (parseFSTree(FSOP_ID_TO_INODE, &objectID, NULL, NULL, &inodeItem, NULL) != 0)
		return 1;

	output->objectID = objectID;
	output->compressed = 0; // TODO: implement this for real

	if (checkHidden)
	{
		if (parseFSTree(FSOP_ID_TO_NAME, &objectID, NULL, NULL, name, NULL) != 0)
			return 1;
		
		output->hidden = (name[0] == '.' ? 1 : 0);
	}
	else
		output->hidden = 0;

	memcpy(&(output->inodeItem), &inodeItem, sizeof(BtrfsInodeItem));

	return 0;
}

int getName(unsigned __int64 objectID, char *output)
{
	if (parseFSTree(FSOP_ID_TO_NAME, &objectID, NULL, NULL, output, NULL) != 0)
		return 1;

	return 0;
}

int dirList(unsigned __int64 objectID, DirList *output)
{
	unsigned __int64 numChildren = 0, *children = NULL, parentID;
	int i;

	if (parseFSTree(FSOP_ID_TO_CHILD_IDS, &objectID, NULL, NULL, &numChildren, &children) != 0)
		return 1;

	output->numEntries = numChildren;
	output->inodes = (Inode *)malloc(sizeof(Inode) * numChildren);
	output->names = (char **)malloc(sizeof(char *) * numChildren);

	for (i = 0; i < numChildren; i++)
	{
		if (i == 0)
			output->names[i] = (char *)malloc(MAX_PATH * numChildren);
		else
			output->names[i] = output->names[0] + (MAX_PATH * i);
		
		if (getName(children[i], output->names[i]) != 0)
			goto error;
		if (getInode(children[i], &(output->inodes[i]), 1) != 0)
			goto error;
	}

	/* for directories other than the root dir, add entries for "." and ".." */
	if (objectID != OBJID_ROOT_DIR)
	{
		output->numEntries += 2;

		output->inodes = (Inode *)realloc(output->inodes, sizeof(Inode) * output->numEntries);
		output->names = (char **)realloc(output->names, sizeof(char **) * output->numEntries);
		output->names[0] = (char *)realloc(output->names[0], MAX_PATH * output->numEntries);

		/* readjust ALL of these because realloc might have moved the block */
		for (i = 0; i < output->numEntries; i++)
			output->names[i] = output->names[0] + (MAX_PATH * i);

		strcpy(output->names[output->numEntries - 2], ".");
		strcpy(output->names[output->numEntries - 1], "..");

		if (getInode(objectID, (&output->inodes[output->numEntries - 2]), 0) != 0)
			return 1;
		if (parseFSTree(FSOP_ID_TO_PARENT_ID, &objectID, NULL, NULL, &parentID, NULL) != 0)
			return 1;
		if (getInode(parentID, (&output->inodes[output->numEntries - 1]), 0) != 0)
			return 1;
	}

	free(children);

	return 0;

error:
	free(children);
	free(output->inodes);
	free(output->names[0]);
	free(output->names);
	return 1;
}

void convertTime(BtrfsTime *bTime, PFILETIME wTime)
{
	LONGLONG s64 = 116444736000000000; // 1601-to-1970 correction factor

	s64 += endian64(bTime->secSince1970) * 10000000;
	s64 += endian32(bTime->nanoseconds) / 100;

	wTime->dwHighDateTime = (DWORD)(s64 >> 32);
	wTime->dwLowDateTime = (DWORD)s64;
}

void destroyDirList(DirList *listing)
{
	free(listing->inodes);
	
	if (listing->numEntries > 0)
		free(listing->names[0]);

	free(listing->names);
}
