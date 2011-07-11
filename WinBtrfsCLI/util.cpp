/* util.cpp
 * utility functions
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

#include "util.h"
#include <cassert>
#include <cstdio>
#include "endian.h"

void convertTime(BtrfsTime *bTime, PFILETIME wTime)
{
	LONGLONG s64 = 116444736000000000; // 1601-to-1970 correction factor

	s64 += endian64(bTime->secSince1970) * 10000000;
	s64 += endian32(bTime->nanoseconds) / 100;

	wTime->dwHighDateTime = (DWORD)(s64 >> 32);
	wTime->dwLowDateTime = (DWORD)s64;
}

void hexToChar(unsigned char hex, char *chr)
{
	static const char digits[] = { '0', '1', '2', '3', '4', '5', '6', '7',
		'8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

	*chr = digits[hex & 0x0f];
}

void uuidToStr(const unsigned char *uuid, char *dest)
{
	size_t len = 0;

	for (size_t i = 0; i < 16; i++)
	{
		if (i == 4 || i == 6 || i == 8 || i == 10)
			dest[len++] = '-';
		
		hexToChar((uuid[i] & 0xf0) >> 4, &dest[len++]);
		hexToChar(uuid[i] & 0x0f, &dest[len++]);
	}

	dest[len] = 0;
}

void stModeToStr(unsigned int mode, char *dest)
{
	if (mode & S_IFDIR)
		dest[0] = 'd';
	else if (mode & S_IFBLK)
		dest[0] = 'b';
	else if (mode & S_IFCHR)
		dest[0] = 'c';
	/* Btrfs seems to mark all files as symlinks for some reason */
	/*else if (mode & S_IFLNK)
		dest[0] = 'l';*/
	else if (mode & S_IFIFO)
		dest[0] = 'p';
	/* Btrfs also appears to enjoy marking everything as a socket */
	/*else if (mode & S_IFSOCK)
		dest[0] = 's';*/
	else
		dest[0] = '-';

	dest[1] = (mode & S_IRUSR) ? 'r' : '-';
	dest[2] = (mode & S_IWUSR) ? 'w' : '-';

	if (mode & S_ISUID)
		dest[3] = (mode & S_IXUSR) ? 's' : 'S';
	else
		dest[3] = (mode & S_IXUSR) ? 'x' : '-';

	dest[4] = (mode & S_IRGRP) ? 'r' : '-';
	dest[5] = (mode & S_IWGRP) ? 'w' : '-';

	if (mode & S_ISGID)
		dest[6] = (mode & S_IXUSR) ? 's' : 'S';
	else
		dest[6] = (mode & S_IXUSR) ? 'x' : '-';

	dest[7] = (mode & S_IROTH) ? 'r' : '-';
	dest[8] = (mode & S_IWOTH) ? 'w' : '-';

	if (mode & S_ISVTX)
		dest[9] = (mode & S_IXUSR) ? 't' : 'T';
	else
		dest[9] = (mode & S_IXUSR) ? 'x' : '-';

	dest[10] = 0;
}

void bgFlagsToStr(BlockGroupFlags flags, char *dest)
{
	/* must be DATA, SYSTEM, or METADATA; nothing else! */
	assert((flags & BGFLAG_DATA) || (flags & BGFLAG_SYSTEM) || (flags & BGFLAG_METADATA));

	if (flags & BGFLAG_DATA)
		strcpy(dest, "DATA");
	else if (flags & BGFLAG_SYSTEM)
		strcpy(dest, "SYSTEM");
	else if (flags & BGFLAG_METADATA)
		strcpy(dest, "METADATA");

	strcat(dest, " ");

	if (flags & BGFLAG_RAID0)
		strcat(dest, "RAID0");
	else if (flags & BGFLAG_RAID1)
		strcat(dest, "RAID1");
	else if (flags & BGFLAG_DUPLICATE)
		strcat(dest, "duplicate");
	else if (flags & BGFLAG_RAID10)
		strcat(dest, "RAID10");
	else
		strcat(dest, "single");
}
