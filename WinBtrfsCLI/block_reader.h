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

#include <Windows.h>
#include "types.h"

#ifndef WINBTRFS_BLOCK_READER_H
#define WINBTRFS_BLOCK_READER_H

class BlockReader
{
public:
	BlockReader(const wchar_t *devicePath);
	~BlockReader();

	DWORD directRead(unsigned __int64 addr, unsigned __int64 len, unsigned char *dest);

private:
	HANDLE hPhysical, hReadMutex;
};

#endif
