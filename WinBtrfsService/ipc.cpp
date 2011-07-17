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
#include <cassert>
#include "log.h"
#include "volume_mgr.h"
#include "WinBtrfsService.h"

namespace WinBtrfsService
{
	extern HANDLE ipcEvent;

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
		CloseHandle(hThread);
	}

	void handleIPC()
	{
		DWORD error, bytesRead = 0, bytesWritten = 0;
		char *buffer = (char *)malloc(10240);
		ServiceMsg *msg;
		
		if ((error = ReadFile(hPipe, buffer, 10240, &bytesRead, NULL)) != 0)
		{
			bool respOK = false, respError = false;
			int errorCode = 0;
			
			msg = (ServiceMsg *)buffer;

			if (msg->type == MSG_REQ_MOUNT)
			{
				MountData *mountData = (MountData *)msg->data;

				assert(msg->dataLen >= sizeof(MountData));
				assert(msg->dataLen >= sizeof(MountData) +
					(mountData->numDevices * sizeof(wchar_t) * MAX_PATH));
				
				log("Received MSG_REQ_MOUNT.\n");
				
				if ((errorCode = mount(mountData)) == 0)
					respOK = true;
				else
					respError = true;
			}

			if (respOK)
			{
				ServiceMsg msg;

				msg.type = MSG_RESP_OK;
				msg.dataLen = 0;
				
				log("Sending MSG_RESP_OK.\n");
				if ((error = WriteFile(hPipe, &msg, sizeof(msg), &bytesWritten, NULL)) == 0)
					log("Attempting to write to the pipe returned error %u: %s",
						error, getErrorMessage(error));
			}

			if (respError)
			{
				ServiceMsg *msg = (ServiceMsg *)malloc(sizeof(ServiceMsg) + sizeof(int));

				msg->type = MSG_RESP_ERROR;
				msg->dataLen = sizeof(int);
				*((int *)msg->data) = errorCode;
				
				log("Sending MSG_RESP_ERROR.\n");
				if ((error = WriteFile(hPipe, &msg, sizeof(msg), &bytesWritten, NULL)) == 0)
					log("Attempting to write to the pipe returned error %u: %s",
						error, getErrorMessage(error));

				free(msg);
			}
		}
		else
			log("Attempting to read from the pipe returned error %u: %s",
			error, getErrorMessage(error));

		free(buffer);
		ResetEvent(ipcEvent);
	}

	/* warning: this function runs in a different thread! */
	DWORD WINAPI watchIPC(LPVOID lpParameter)
	{
		do
		{
			BOOL result = ConnectNamedPipe(hPipe, NULL);
			DWORD error = GetLastError();
			
			if (result != 0 || error == ERROR_PIPE_CONNECTED)
			{
				log("Got a connection on the named pipe.\n");
				SetEvent(ipcEvent);
			}
			else
			{
				DWORD error = GetLastError();
				
				switch (error)
				{
				case ERROR_NO_DATA:
					/* the client is done with the pipe */
					log("Disconnecting from the named pipe.\n");
					DisconnectNamedPipe(hPipe);
					break;
				case ERROR_IO_PENDING:
				case ERROR_PIPE_LISTENING:
					/* not a problem */
					break;
				default:
					log("ConnectNamedPipe returned error %u: %s", error, getErrorMessage(error));
					break;
				}
			}
		}
		while (WaitForSingleObject(threadTermEvent, 100) != WAIT_OBJECT_0);

		/* we're done here */
		SetEvent(threadDoneEvent);

		return 0;
	}
}
