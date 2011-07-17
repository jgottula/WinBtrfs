/* ThreadLocalTest/main.cpp
 * testbed for determining the specifics of TLS
 *
 * WinBtrfs
 * Copyright (c) 2011 Justin Gottula
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <cassert>
#include <cstdio>
#include "thread.h"

__declspec(thread) int tlsVar = 10;

int main(int argc, char **argv)
{
	HANDLE hTh1, hTh2;
	
	printf("main: tlsVar = %d\n", tlsVar);

	assert((hTh1 = CreateThread(NULL, 0, &threadProc, (LPVOID)1, 0, NULL)) != NULL);
	Sleep(100);
	assert((hTh2 = CreateThread(NULL, 0, &threadProc, (LPVOID)2, 0, NULL)) != NULL);
	Sleep(100);
	
	printf("main: tlsVar = %d\n", tlsVar);

	return 0;
}
