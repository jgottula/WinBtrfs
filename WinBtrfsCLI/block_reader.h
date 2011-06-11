/* block_reader.h
 * low-level block reading and caching
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

#include <map>
#include <Windows.h>

struct CacheNode
{
	CacheNode()
	{
		numReads = 0;
	}
	
	unsigned __int64 numReads;
	unsigned __int64 size;
	unsigned char *data;
};

class BlockReader
{
public:
	BlockReader();
	~BlockReader();

	DWORD cachedRead(unsigned __int64 addr, int addrType, unsigned __int64 len, unsigned char *dest);
	DWORD directRead(unsigned __int64 addr, int addrType, unsigned __int64 len, unsigned char *dest);

private:
	std::map<unsigned __int64, CacheNode> nodeArr;
	HANDLE hPhysical, hReadMutex;
};
