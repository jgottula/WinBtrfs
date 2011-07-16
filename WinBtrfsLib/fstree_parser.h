/* WinBtrfsLib/fstree_parser.h
 * fs tree parser
 *
 * WinBtrfs
 * Copyright (c) 2011 Justin Gottula
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "types.h"

int parseFSTree(BtrfsObjID tree, FSOperation operation, void *input0, void *input1, void *input2,
	void *output0, void *output1);
