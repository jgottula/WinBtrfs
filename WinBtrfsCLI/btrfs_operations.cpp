/* btrfs_operations.cpp
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
#include "chunktree_parser.h"
#include "roottree_parser.h"
#include "fstree_parser.h"

extern BtrfsObjID defaultSubvol;

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

int getPathID(const char *path, BtrfsObjID *output)
{
	char vPath[MAX_PATH], **components;
	BtrfsObjID parentID = OBJID_ROOT_DIR, childID;
	unsigned int hash;
	int numComponents = -1;

	validatePath(path, vPath);
	numComponents = componentizePath(path, &components);

	for (int i = 0; i < numComponents; i++)
	{
		hash = crc32c((unsigned int)~1, (const unsigned char *)(components[i]), strlen(components[i]));
		
		if (parseFSTree(defaultSubvol, FSOP_NAME_TO_ID, &parentID, &hash, components[i],
			&childID, NULL) != 0)
			return 1;

		parentID = childID;
	}

	*output = parentID;

	return 0;
}
