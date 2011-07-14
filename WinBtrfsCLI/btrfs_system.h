/* btrfs_system.h
 * low-level filesystem code
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

#include <vector>
#include <Windows.h>
#include "structures.h"

DWORD init(std::vector<const wchar_t *>& devicePaths);
void cleanUp();
unsigned __int64 logiToPhys(unsigned __int64 logiAddr, unsigned __int64 len);
DWORD readPrimarySB();
int validateSB(BtrfsSuperblock *s);
int findSecondarySBs();
void loadSBChunks(bool dump);
unsigned char *loadNode(unsigned __int64 blockAddr, AddrType type, BtrfsHeader **header);
unsigned __int64 getTreeRootAddr(BtrfsObjID tree);
