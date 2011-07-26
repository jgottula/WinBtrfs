/* WinBtrfsDrv/main.cpp
 * filesystem instance startup code
 *
 * WinBtrfs
 * Copyright (c) 2011 Justin Gottula
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <cstdio>
#include <Windows.h>

namespace WinBtrfsDrv
{
	void improperInvocation()
	{
		fprintf(stderr, "WinBtrfsDrv must be run by WinBtrfsService.\n");
		exit(1);
	}
}

using namespace WinBtrfsDrv;

int main(int argc, char **argv)
{
	if (argc != 3 || strlen(argv[1]) <= 12 || strncmp(argv[1], "--pipe-name=", 12) != 0 ||
		strlen(argv[2]) <= 13 || strncmp(argv[2], "--parent-pid=", 13) != 0)
		improperInvocation();
	
	const char *pipeName = argv[1] + 12;
	int parentPID = -1;

	if (sscanf(argv[2] + 13, "%d", &parentPID) != 1)
		improperInvocation();

	printf("pipeName: %s\nparentPID: %d\n", pipeName, parentPID);
	Sleep(5000);

	return 0;
}
