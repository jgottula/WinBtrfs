/* block_reader.cpp
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

#include "block_reader.h"
#include "btrfs_system.h"

extern WCHAR devicePath[MAX_PATH];

BlockReader::BlockReader()
{
	hPhysical = CreateFile(devicePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	assert(hPhysical != INVALID_HANDLE_VALUE);
	hReadMutex = CreateMutex(NULL, FALSE, NULL);
	assert(hReadMutex != INVALID_HANDLE_VALUE);

	cacheSize = 0;
}

BlockReader::~BlockReader()
{
	CloseHandle(hPhysical);
	CloseHandle(hReadMutex);
}

/* consolidate cachedRead and directRead in the future */
DWORD BlockReader::cachedRead(unsigned __int64 addr, AddrType type, unsigned __int64 len,
	boost::shared_array<unsigned char> *out)
{
	std::list<CacheNode>::iterator it, end = nodeArr.end();
	bool foundNode = false;
	LARGE_INTEGER li;
	DWORD bytesRead;

	if (type == ADDR_LOGICAL)
		addr = logiToPhys(addr, len);

	for (it = nodeArr.begin(); it != end; ++it)
	{
		if (it->physAddr == addr && it->size == len)
		{
			foundNode = true;
			break;
		}
	}

	if (!foundNode) // cache miss
	{
		CacheNode cNode;

		cNode.numReads = 1;
		cNode.physAddr = addr;
		cNode.size = len;

		/* because Win32 uses a signed (??) 64-bit value for the address from which to read,
			our address space is cut in half from what Btrfs technically allows */
		li.QuadPart = (LONGLONG)addr;

		/* using SetFilePointerEx and then ReadFile is clearly not threadsafe, so for now
			I'm using a mutex here; in the future, this should be fully threaded
			(perhaps a separate read handle per thread? this would require a CreateFile on each
			call—or perhaps declare a static local that would retain value per thread?) */
		if (WaitForSingleObject(hReadMutex, 10000) != WAIT_OBJECT_0)
			return GetLastError();

		if (SetFilePointerEx(hPhysical, li, NULL, 0) == 0)
		{
			ReleaseMutex(hReadMutex);
			return GetLastError();
		}

		cNode.data = new boost::shared_array<unsigned char>(new unsigned char[len]);

		if (ReadFile(hPhysical, cNode.data->get(), (DWORD)len, &bytesRead, NULL) == 0 || bytesRead != len)
		{
			ReleaseMutex(hReadMutex);
			delete cNode.data;
			return GetLastError();
		}

		ReleaseMutex(hReadMutex);
		
		/* increase the cache size, then purge BEFORE adding the new node so we never
			really exceed the limit */
		cacheSize += len;
		purge();

		/* try to insert this element at the BEGINNING of all the numReads==1 elements
			(see the purge member function for more details about why) */
		for (it = nodeArr.begin(); it != end; ++it)
		{
			if (it->numReads == cNode.numReads)
				break;
		}
		
		/* this does a copy operation, which is OK here */
		nodeArr.insert(it, cNode);

		/* create a new reference to the shared_array */
		*out = *cNode.data;

		/*printf("BlockReader::cachedRead: miss!\n");
		dump();*/
		return 0;
	}
	else // cache hit
	{
		CacheNode& cNode = *it;

		cNode.numReads++;

		for (std::list<CacheNode>::iterator itS = nodeArr.begin(); itS != it; ++itS)
		{
			if (itS->numReads < cNode.numReads) // this node needs to be moved up
			{
				/* move it up! */
				nodeArr.splice(itS, nodeArr, it);

				break;
			}
		}
		
		/* create a new reference to the shared_array */
		*out = *cNode.data;
		
		/*printf("BlockReader::cachedRead: hit!\n");
		dump();*/
		return 0;
	}
}

DWORD BlockReader::directRead(unsigned __int64 addr, AddrType type, unsigned __int64 len, unsigned char *dest)
{
	LARGE_INTEGER li;
	DWORD bytesRead;

	if (type == ADDR_LOGICAL)
		addr = logiToPhys(addr, len);

	/* because Win32 uses a signed (??) 64-bit value for the address from which to read,
		our address space is cut in half from what Btrfs technically allows */
	li.QuadPart = (LONGLONG)addr;

	/* using SetFilePointerEx and then ReadFile is clearly not threadsafe, so for now
		I'm using a mutex here; in the future, this should be fully threaded
		(perhaps a separate read handle per thread? this would require a CreateFile on each
		call—or perhaps declare a static local that would retain value per thread?) */
	if (WaitForSingleObject(hReadMutex, 10000) != WAIT_OBJECT_0)
		return GetLastError();

	if (SetFilePointerEx(hPhysical, li, NULL, 0) == 0)
	{
		ReleaseMutex(hReadMutex);
		return GetLastError();
	}

	if (ReadFile(hPhysical, dest, (DWORD)len, &bytesRead, NULL) == 0 || bytesRead != len)
	{
		ReleaseMutex(hReadMutex);
		return GetLastError();
	}

	ReleaseMutex(hReadMutex);

	return 0;
}

void BlockReader::dump()
{
	printf("BlockReader::dump: nodeArr.size() = %d cacheSize = %d\n", nodeArr.size(), cacheSize);
	
	std::list<CacheNode>::iterator it = nodeArr.begin(), end = nodeArr.end();
	for (int i = 0; it != end; i++, ++it)
		printf("%3d] numReads: %I64d size: %I64d physAddr: %I64x\n", i, it->numReads, it->size,
			it->physAddr);

	printf("\n");
}

void BlockReader::purge()
{
	while (nodeArr.size() > 0 && cacheSize > MAX_CACHE_SIZE)
	{
		/* this function assumes that (a) elements are sorted from most reads to least, and that
			(b) when elements have equal numbers of reads, they are sorted from most recent to least */
		CacheNode& last = nodeArr.back();

		cacheSize -= last.size;

		/* remove a reference to the shared_ptr */
		delete last.data;

		nodeArr.pop_back();
	}

	/*printf("BlockReader::purge: removed least-used nodes.\n");
	dump();*/
}
