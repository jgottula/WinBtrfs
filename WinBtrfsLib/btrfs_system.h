/* WinBtrfsLib/btrfs_system.h
 * low-level filesystem code
 *
 * WinBtrfs
 * Copyright (c) 2011 Justin Gottula
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <Windows.h>
#include "block_reader.h"
#include "types.h"

namespace WinBtrfsLib
{
	void allocateBlockReaders();
	void cleanUp();
	PhysAddr *logiToPhys(LogiAddr logiAddr, unsigned __int64 len);
	int loadSBs(bool dump);
	int validateSB(BtrfsSuperblock *s);
	void loadSBChunks(bool dump);
	unsigned char *loadNode(LogiAddr addr, BtrfsHeader **header);
	LogiAddr getTreeRootAddr(BtrfsObjID tree);
	int verifyDevices();
	BlockReader *getBlockReader(unsigned __int64 devID);
	DWORD readLogical(LogiAddr addr, unsigned __int64 len, unsigned char *dest);
}
