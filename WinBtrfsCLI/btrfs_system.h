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

DWORD init();
void cleanUp();
unsigned __int64 logiToPhys(unsigned __int64 logiAddr, unsigned __int64 len);
DWORD readBlock(unsigned __int64 addr, int addrType, unsigned __int64 len, unsigned char *dest);
DWORD readPrimarySB();
int validateSB(BtrfsSuperblock *s);
int findSecondarySBs();
void getSBChunks();
void loadNode(unsigned __int64 blockAddr, int addrType, unsigned char **nodeDest, BtrfsHeader **header);
void parseChunkTree();
void parseRootTree();
int parseFSTree(int operation, void *input1, void *input2, void *input3, void *output1, void *output2);
unsigned __int64 getFSRootBlockNum();
void dump();
