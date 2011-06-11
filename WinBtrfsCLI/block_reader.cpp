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

#include <assert.h>
#include "constants.h"
#include "btrfs_system.h"
#include "block_reader.h"

extern WCHAR devicePath[MAX_PATH];

BlockReader::BlockReader()
{
	/* gotta love C++ and its stupid constructor pointer/reference bullshit */
	nodeArr = *(new std::map<unsigned __int64, CacheNode>());

	hPhysical = CreateFile(devicePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	assert(hPhysical != INVALID_HANDLE_VALUE);
	hReadMutex = CreateMutex(NULL, FALSE, NULL);
	assert(hReadMutex != INVALID_HANDLE_VALUE);
}

BlockReader::~BlockReader()
{
	CloseHandle(hPhysical);
	CloseHandle(hReadMutex);
}

/* consolidate cachedRead and directRead in the future */
DWORD BlockReader::cachedRead(unsigned __int64 addr, int addrType, unsigned __int64 len, unsigned char *dest)
{
	LARGE_INTEGER li;
	DWORD bytesRead;

	if (addrType == ADDR_LOGICAL)
		addr = logiToPhys(addr, len);

	CacheNode& cNode = nodeArr[addr];

	if (cNode.numReads == 0 || cNode.size != len) // cache miss
	{
		if (cNode.numReads != 0) // old node
			free(cNode.data);
		
		cNode.numReads = 0;
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

		cNode.data = (unsigned char *)malloc(len);

		if (ReadFile(hPhysical, cNode.data, (DWORD)len, &bytesRead, NULL) == 0 || bytesRead != len)
		{
			ReleaseMutex(hReadMutex);
			free(cNode.data);
			return GetLastError();
		}

		ReleaseMutex(hReadMutex);
	}

	memcpy(dest, cNode.data, len);
	cNode.numReads++;

	return 0;
}

DWORD BlockReader::directRead(unsigned __int64 addr, int addrType, unsigned __int64 len, unsigned char *dest)
{
	LARGE_INTEGER li;
	DWORD bytesRead;

	if (addrType == ADDR_LOGICAL)
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
