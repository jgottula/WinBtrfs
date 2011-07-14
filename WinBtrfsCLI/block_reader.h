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

#include <list>
#include <boost/shared_array.hpp>
#include <Windows.h>
#include "constants.h"

struct CacheNode
{
	unsigned __int64 numReads;
	unsigned __int64 physAddr;
	unsigned __int64 size;
	boost::shared_array<unsigned char> *data;
};

class BlockReader
{
public:
	BlockReader(const wchar_t *devicePath);
	~BlockReader();

	DWORD cachedRead(unsigned __int64 addr, AddrType type, unsigned __int64 len, boost::shared_array<unsigned char> *out);
	DWORD directRead(unsigned __int64 addr, AddrType type, unsigned __int64 len, unsigned char *dest);
	void dump();

private:
	static const unsigned __int64 MAX_CACHE_SIZE = 8 * 1024 * 1024; // 8 MiB

	std::list<CacheNode> nodeArr;
	HANDLE hPhysical, hReadMutex;
	unsigned __int64 cacheSize;

	void purge();
};
