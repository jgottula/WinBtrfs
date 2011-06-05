/* btrfs_filesystem.h
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

DWORD init();
void cleanUp();
unsigned __int64 logiToPhys(unsigned __int64 logiAddr, unsigned __int64 len);
DWORD readBlock(LONGLONG physAddr, DWORD len, LPVOID dest);
DWORD readLogicalBlock(LONGLONG logiAddr, DWORD len, LPVOID dest);
DWORD readPrimarySB();
int validateSB(BtrfsSuperblock *s);
int findSecondarySBs();
void getSBChunks();
void parseNodePhysical(unsigned __int64 physAddr);
void parseNodeLogical(unsigned __int64 logiAddr);
void parseChunkTree();
void parseRootTree();
void parseFSTree();
void dump();
