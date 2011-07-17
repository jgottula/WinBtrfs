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
	HANDLE hPipe = INVALID_HANDLE_VALUE, hThread = INVALID_HANDLE_VALUE,
		threadTermEvent = INVALID_HANDLE_VALUE, threadDoneEvent = INVALID_HANDLE_VALUE;
	
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

		if ((hThread = CreateThread(NULL, 0, &watchIPC, NULL, 0, NULL)) == NULL)
		{
			DWORD error = GetLastError();
			
			log("CreateThread returned error %u: %s", error, getErrorMessage(error));
			
			return error;
		}

		threadTermEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		threadDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

		return 0;
	}

	void cleanUpIPC()
	{
		/* give the thread a full second to finish cleanly; then, mercilessly terminate it */
		SetEvent(threadTermEvent);
		if (WaitForSingleObject(threadDoneEvent, 1000) == WAIT_TIMEOUT)
			TerminateThread(hThread, 1);
		
		CloseHandle(hPipe);
		CloseHandle(threadTermEvent);
		CloseHandle(threadDoneEvent);
	}

	void handleIPC()
	{
		/* handle incoming IPC events */
		
		/* unset the ipcEvent event when all IPC messages have been handled */
	}

	/* warning: this function runs in a different thread! */
	DWORD WINAPI watchIPC(LPVOID lpParameter)
	{
		/* watch for connections on the named pipe, and if one comes in,
			set the ipcEvent event. */

		/* immediately terminate if the threadTermEvent event is set. */
		/* set threadDoneEvent when we're finished here. */

		return 0;
	}
}
