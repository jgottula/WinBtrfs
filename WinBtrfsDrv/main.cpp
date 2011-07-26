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
#include "init.h"
#include "ipc.h"

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
	printf("WinBtrfsDrv | Copyright (c) 2011 Justin Gottula\n"
		"Please report bugs at http://github.com/jgottula/WinBtrfs\n\n\n");

	/* check arguments for validity */
	if (argc != 4 || strlen(argv[1]) <= 5 || strncmp(argv[1], "--id=", 5) != 0 ||
		strlen(argv[2]) <= 12 || strncmp(argv[2], "--pipe-name=", 12) != 0 ||
		strlen(argv[3]) <= 13 || strncmp(argv[3], "--parent-pid=", 13) != 0)
		improperInvocation();
	
	/* load argument values */
	if (sscanf(argv[1] + 5, "%I64x", &instanceID) != 1)
		improperInvocation();
	wcscpy(pipeName, L"\\\\.\\pipe\\");
	mbstowcs(pipeName + 9, argv[2] + 12, MAX_PATH - 9);
	if (sscanf(argv[3] + 13, "%d", &parentPID) != 1)
		improperInvocation();
	
	/* store parent process as a handle instead of PID;
		this will help with possible termination of the parent */
	hParentProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, parentPID);

	/* sleep for now */
	Sleep(10000);

	const wchar_t msg[] = L"DrvMountData";
	wchar_t buffer[51200];
	size_t bufWritten;
	sendMessage(msg, buffer, sizeof(buffer), &bufWritten);

	/* TODO: shove the data we received into mountData (extern from init.cpp)
		then, call init */

	return 0;
}
