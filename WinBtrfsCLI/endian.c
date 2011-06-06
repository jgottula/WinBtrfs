/* endian.c
 * functions to deal with disk/CPU endianness differences
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

#include <intrin.h>
#include <assert.h>

int archIsBigEndian = -1;

void endianDetect()
{
	union
	{
		unsigned int l;
		unsigned char c[4];
	} bigint = { 0x01020304 };
	const char be[] = { 0x01, 0x02, 0x03, 0x04 },
		le[] = { 0x04, 0x03, 0x02, 0x01 };

	if (memcmp(bigint.c, be, 4) == 0)
		archIsBigEndian = 1;
	else if (memcmp(bigint.c, le, 4) == 0)
		archIsBigEndian = 0;

	/* middle endian?! */
	assert(archIsBigEndian != -1);
}

unsigned short endian16(unsigned short leFromDisk)
{
	return (archIsBigEndian ? _byteswap_ushort(leFromDisk) : leFromDisk);
}

unsigned int endian32(unsigned int leFromDisk)
{
	return (archIsBigEndian ? _byteswap_ulong(leFromDisk) : leFromDisk);
}

unsigned __int64 endian64(unsigned __int64 leFromDisk)
{
	return (archIsBigEndian ? _byteswap_uint64(leFromDisk) : leFromDisk);
}
