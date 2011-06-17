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

#include <stdio.h>
#include <Windows.h>
#include "structures.h"
#include "endian.h"

void convertTime(BtrfsTime *bTime, PFILETIME wTime)
{
	LONGLONG s64 = 116444736000000000; // 1601-to-1970 correction factor

	s64 += endian64(bTime->secSince1970) * 10000000;
	s64 += endian32(bTime->nanoseconds) / 100;

	wTime->dwHighDateTime = (DWORD)(s64 >> 32);
	wTime->dwLowDateTime = (DWORD)s64;
}

void uuidToStr(const unsigned char *uuid, char *dest)
{
	char temp[1024];

	strcpy(dest, "{");

	for (int i = 0; i < 16; i++)
	{
		if (i == 4 || i == 6 || i == 8 || i == 10)
			strcat(dest, "-");
		
		sprintf(temp, "%02x", uuid[i]);
		strcat(dest, temp);
	}

	strcat(dest, "}");
}
