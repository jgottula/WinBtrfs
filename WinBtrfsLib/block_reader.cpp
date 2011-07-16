/* WinBtrfsLib/block_reader.cpp
 * low-level block reading and caching
 *
 * WinBtrfs
 * Copyright (c) 2011 Justin Gottula
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "block_reader.h"
#include <cassert>
#include "btrfs_system.h"

BlockReader::BlockReader(const wchar_t *devicePath)
{
	hPhysical = CreateFile(devicePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	/* this is NOT fatal; return an error based on GetLastError (file not found is most common) */
	assert(hPhysical != INVALID_HANDLE_VALUE);
	hReadMutex = CreateMutex(NULL, FALSE, NULL);
	assert(hReadMutex != INVALID_HANDLE_VALUE);
}

BlockReader::~BlockReader()
{
	CloseHandle(hPhysical);
	CloseHandle(hReadMutex);
}

DWORD BlockReader::directRead(unsigned __int64 addr, unsigned __int64 len, unsigned char *dest)
{
	LARGE_INTEGER li;
	DWORD bytesRead;

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
