/* WinBtrfsLib/instance.h
 * infrastructure to handle multiple instances
 *
 * WinBtrfs
 * Copyright (c) 2011 Justin Gottula
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#ifndef WINBTRFSLIB_INSTANCE_H
#define WINBTRFSLIB_INSTANCE_H

#include <list>
#include <map>
#include <vector>
#include "block_reader.h"
#include "WinBtrfsLib.h"

namespace WinBtrfsLib
{
	struct InstanceData
	{
		MountData *mountData;
		BtrfsObjID mountedSubvol;
		std::vector<KeyedItem> chunkTree;
		std::vector<BlockReader *> blockReaders;
		std::vector<BtrfsSuperblock> supers;
		std::vector<BtrfsSBChunk *> sbChunks; // using an array of ptrs because BtrfsSBChunk is variably sized
		std::list<FilePkg> openFiles, cleanedUpFiles;
		HANDLE hBigDokanLock;
	};

	extern std::map<DWORD, InstanceData *> instances;

	InstanceData *getThisInst();
}

#endif
