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
DWORD readBlock(LONGLONG physAddr, DWORD len, LPVOID dest);
DWORD readLogicalBlock(LONGLONG logiAddr, DWORD len, LPVOID dest);
DWORD readPrimarySB();
int validateSB(BtrfsSuperblock *s);
int findSecondarySBs();
void getSBChunks();
void parseChunkTree();
void parseRootTree();
