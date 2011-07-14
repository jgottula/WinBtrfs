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

#include <Windows.h>
#include "structures.h"

DWORD init();
void cleanUp();
unsigned __int64 logiToPhys(unsigned __int64 logiAddr, unsigned __int64 len);
int loadSBs();
int validateSB(BtrfsSuperblock *s);
void loadSBChunks(bool dump);
unsigned char *loadNode(unsigned __int64 blockAddr, AddrType type, BtrfsHeader **header);
unsigned __int64 getTreeRootAddr(BtrfsObjID tree);
int verifyDevices();
