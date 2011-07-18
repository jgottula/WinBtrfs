/* WinBtrfsLib/btrfs_operations.cpp
 * high-level filesystem operations
 *
 * WinBtrfs
 * Copyright (c) 2011 Justin Gottula
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "btrfs_operations.h"
#include "crc32c.h"
#include "fstree_parser.h"
#include "instance.h"

namespace WinBtrfsLib
{
	void validatePath(const char *input, char *output)
	{
		size_t c = 0, len = strlen(input);

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

		for (size_t i = 0; i < len; i++)
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
	unsigned int componentizePath(const char *path, char ***output)
	{
		size_t len = strlen(path), compLen = 0;
		int numComponents = 0, compIdx = 0, compC = 0;

		if (len == 1 && path[0] == '\\')
			return 0;

		for (size_t i = 0; i < len; i++)
		{
			if (path[i] == '\\')
				numComponents++;
		}

		/* allocate the array of pointers */
		*output = (char **)malloc(sizeof(char *) * numComponents);

		for (size_t i = 1; i < len; i++) // skip the first backslash
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

		for (size_t i = 1; i < len; i++) // again, skip the first backslash
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

	int getPathID(const char *path, FileID *output, FileID *parent)
	{
		InstanceData *thisInst = getThisInst();
		char vPath[MAX_PATH], **components;
		FileID fileID, childID;
		unsigned int numComponents;
		unsigned int hash;
		bool isSubvolume;

		validatePath(path, vPath);
		numComponents = componentizePath(path, &components);

		/* start at the root directory of the currently mounted subvolume */
		fileID.treeID = childID.treeID = thisInst->mountedSubvol;
		fileID.objectID = childID.objectID = OBJID_ROOT_DIR;

		for (int i = 0; i < numComponents; i++)
		{
			/* ratchet up */
			memcpy(&fileID, &childID, sizeof(FileID));

			hash = crc32c((unsigned int)~1, (const unsigned char *)(components[i]), strlen(components[i]));
		
			if (parseFSTree(fileID.treeID, FSOP_NAME_TO_ID, &fileID.objectID, &hash, components[i],
				&childID.objectID, &isSubvolume) != 0)
				return 1;

			if (isSubvolume)
			{
				/* reset to the root of this subvolume's tree */
				childID.treeID = childID.objectID;
				childID.objectID = OBJID_ROOT_DIR;
			}
		}

		memcpy(output, &childID, sizeof(FileID));
		memcpy(parent, &fileID, sizeof(FileID));

		return 0;
	}
}
