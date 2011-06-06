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

int getInode(const char *path, Inode *inode)
{
	char vPath[MAX_PATH], **components;
	unsigned __int64 parentID = OBJID_ROOT_DIR, childID, hash;
	int i, numComponents = -1;
	BtrfsInodeItem inodeItem;

	validatePath(path, vPath);
	numComponents = componentizePath(path, &components);

	for (i = 0; i < numComponents; i++)
	{
		childID = -1;
		hash = crc32c(0, (const unsigned char *)(components[i]), strlen(components[i]));
		parseFSTree(FSOP_NAME_TO_ID, &parentID, &hash, components[i], &childID);

		if (childID == -1)
			return 1;

		parentID = childID;
	}

	parseFSTree(FSOP_ID_TO_INODE, &parentID, NULL, NULL, &inodeItem);

	inode->objectID = parentID;
	inode->hidden = (numComponents > 0 && components[numComponents - 1][0] == '.') ? 1 : 0;
	inode->compressed = 0; // TODO: implement this for real

	memcpy(&(inode->inodeItem), &inodeItem, sizeof(BtrfsInodeItem));

	return 0;
}

void convertTime(BtrfsTime *bTime, PFILETIME wTime)
{
	LONGLONG s64 = 116444736000000000; // 1601-to-1970 correction factor

	s64 += endian64(bTime->secSince1970) * 10000000;
	s64 += endian32(bTime->nanoseconds) / 100;

	wTime->dwHighDateTime = (DWORD)(s64 >> 32);
	wTime->dwLowDateTime = (DWORD)s64;
}
