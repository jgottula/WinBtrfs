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

int archIsBigEndian = -1;

void endianDetect()
{
	union
	{
		unsigned short s;
		unsigned char c[2];
	} bigint = { 0x0102 };

	archIsBigEndian = (bigint.c[0] == 1);
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
