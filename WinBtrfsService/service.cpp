/* WinBtrfsService/service.cpp
 * Windows service boilerplate
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
#include <Windows.h>
#include "log.h"
#include "volume_mgr.h"

namespace WinBtrfsService
{
	SERVICE_STATUS status;
	SERVICE_STATUS_HANDLE hStatus;
	HANDLE stopEvent;
	
	DWORD WINAPI serviceCtrlHandlerEx(DWORD dwControl, DWORD dwEventType,
		LPVOID lpEventData, LPVOID lpContext)
	{
		switch (dwControl)
		{
		case SERVICE_CONTROL_INTERROGATE:
			return NO_ERROR;
		case SERVICE_CONTROL_STOP:
		case SERVICE_CONTROL_SHUTDOWN:
			status.dwCurrentState = SERVICE_STOP_PENDING;
			SetServiceStatus(hStatus, &status);

			/* notify serviceMain that it needs to clean up */
			SetEvent(stopEvent);

			return NO_ERROR;
		default:
			return ERROR_CALL_NOT_IMPLEMENTED;
		}
	}
	
	void WINAPI serviceMain(DWORD dwNumServicesArgs, LPWSTR *lpServiceArgVectors)
	{
		status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
		status.dwCurrentState = SERVICE_STOPPED;
		status.dwControlsAccepted = 0;
		status.dwWin32ExitCode = NO_ERROR;
		status.dwServiceSpecificExitCode = NO_ERROR;
		status.dwCheckPoint = 0;
		status.dwWaitHint = 0;
		
		if ((hStatus = RegisterServiceCtrlHandlerEx(L"WinBtrfsService",
			&serviceCtrlHandlerEx, NULL)) != 0)
		{
			status.dwCurrentState = SERVICE_START_PENDING;
			SetServiceStatus(hStatus, &status);

			/* global initialization tasks */
			stopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

			status.dwControlsAccepted |= SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
			status.dwCurrentState = SERVICE_RUNNING;
			SetServiceStatus(hStatus, &status);

			do
			{
				checkIPC();
			}
			while (WaitForSingleObject(stopEvent, 10000) == WAIT_TIMEOUT);

			/* global cleanup tasks */
			CloseHandle(stopEvent);
			unmountAll();

			status.dwControlsAccepted = 0;
			status.dwCurrentState = SERVICE_STOPPED;
			SetServiceStatus(hStatus, &status);
		}
	}
}

using namespace WinBtrfsService;

int main(int argc, char **argv)
{
	DWORD error = 0;
	SERVICE_TABLE_ENTRY serviceTable[] =
	{
		{ L"WinBtrfsService", &WinBtrfsService::serviceMain },
		{ NULL, NULL }
	};

	logInit();

	/* this function will not return until all of this process's services have stopped */
	if (StartServiceCtrlDispatcher(serviceTable) == 0)
	{
		char message[1024];
		
		error = GetLastError();
		
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, message, 1024, NULL);
		log("StartServiceCtrlDispatcher returned error %u: %s", error, message);

		/* this would indicate a grave error on the part of the developer */
		assert(error != ERROR_INVALID_DATA);

		/* boneheaded user error */
		if (error == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT)
			fprintf(stderr, "Hey, you can't do that! This program must be run as a service!\n");
	}

	log("Terminated with code %u.\n", error);
	logClose();

	return error;
}
