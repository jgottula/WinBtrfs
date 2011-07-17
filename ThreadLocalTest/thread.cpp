/* ThreadLocalTest/thread.cpp
 * TLS thread callbacks
 *
 * WinBtrfs
 * Copyright (c) 2011 Justin Gottula
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "thread.h"
#include <cstdio>

extern __declspec(thread) int tlsVar;

DWORD WINAPI threadProc(LPVOID lpParameter)
{
	printf("threadProc [%d]: tlsVar = %d\n", (int)lpParameter, tlsVar);
	printf("threadProc [%d]: ++tlsVar = %d\n", (int)lpParameter, ++tlsVar);

	return 0;
}
