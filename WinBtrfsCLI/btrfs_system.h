/* btrfs_system.h
 * low-level filesystem operations
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

#include <boost/shared_array.hpp>
#include <Windows.h>
#include "structures.h"

DWORD init();
void cleanUp();
unsigned __int64 logiToPhys(unsigned __int64 logiAddr, unsigned __int64 len);
DWORD readPrimarySB();
int validateSB(BtrfsSuperblock *s);
int findSecondarySBs();
void loadSBChunks();
boost::shared_array<unsigned char> *loadNode(unsigned __int64 blockAddr, int addrType, BtrfsHeader **header);
void parseChunkTree();
void parseRootTree();
int parseFSTree(FSOperation operation, void *input1, void *input2, void *input3, void *output1, void *output2);
unsigned __int64 getFSRootBlockNum();
