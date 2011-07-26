/* WinBtrfsDrv/ipc.cpp
 * interprocess communications
 *
 * WinBtrfs
 * Copyright (c) 2011 Justin Gottula
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "ipc.h"
#include <cstdio>

namespace WinBtrfsDrv
{
	unsigned __int64 instanceID;
	wchar_t pipeName[MAX_PATH];
	int parentPID;
	HANDLE hParentProc;

	int sendMessage(const wchar_t *msg, wchar_t *buffer, size_t bufLen, size_t *bufWritten)
	{
		HANDLE hPipe = INVALID_HANDLE_VALUE;

		if (WaitNamedPipe(pipeName, 1000) == 0)
		{
			fprintf(stderr, "sendMessage: WaitNamedPipe failed: %u\n", GetLastError());

			return 1;
		}

		if ((hPipe = CreateFile(pipeName, GENERIC_READ | GENERIC_WRITE,
			0, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
		{
			fprintf(stderr, "sendMessage: CreateFile failed: %u\n", GetLastError());

			return 2;
		}
		
		DWORD bytesWritten = 0;
		if (WriteFile(hPipe, msg, wcslen(msg), &bytesWritten, NULL) == 0)
		{
			fprintf(stderr, "sendMessage: WriteFile failed: %u\n", GetLastError());

			CloseHandle(hPipe);
			return 3;
		}

		/* TODO: make sure that this function (a) waits for at least a second, and (b) doesn't block indefinitely */
		DWORD bytesRead = 0;
		if (ReadFile(hPipe, buffer, bufLen - 1, &bytesRead, NULL) == 0)
		{
			fprintf(stderr, "sendMessage: ReadFile failed: %u\n", GetLastError());

			CloseHandle(hPipe);
			return 4;
		}

		/* null terminate the string we get */
		((wchar_t *)buffer)[bytesRead / 2] = 0;

		*bufWritten = bytesRead;
		
		CloseHandle(hPipe);
		return 0;
	}
}
