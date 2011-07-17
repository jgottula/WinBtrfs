/* WinBtrfsService/ipc.cpp
 * inter-process communication
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
#include "log.h"

namespace WinBtrfsService
{
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	
	DWORD setUpIPC()
	{
		if ((hPipe = CreateNamedPipe(L"\\\\.\\pipe\\WinBtrfsService", PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_NOWAIT, PIPE_UNLIMITED_INSTANCES,
			1024, 1024, 0, NULL)) == INVALID_HANDLE_VALUE)
		{
			DWORD error = GetLastError();
			
			log("CreateNamedPipe returned error %u: %s", error, getErrorMessage(error));
			
			return error;
		}

		// create thread for watchIPC

		return 0;
	}

	void cleanUpIPC()
	{
		// end the thread
		CloseHandle(hPipe);
	}

	/* warning: this function runs in a different thread! */
	DWORD WINAPI watchIPC(LPVOID lpParameter)
	{
		/* watch for connections on the named pipe, and if one comes in,
			set the ipcEvent event. */

		return 0;
	}

	void handleIPC()
	{
		/* handle incoming IPC events */
		
		/* unset the ipcEvent event when all IPC messages have been handled */
	}
}
