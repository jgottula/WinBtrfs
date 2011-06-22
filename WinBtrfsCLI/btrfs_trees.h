/* btrfs_trees.h
 * tree parsers
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

#include "structures.h"

void parseChunkTree(CTOperation operation);
void parseRootTree(RTOperation operation);
int parseFSTree(BtrfsObjID tree, FSOperation operation, void *input1, void *input2, void *input3,
	void *output1, void *output2);
unsigned __int64 getTreeRootAddr(BtrfsObjID tree);
