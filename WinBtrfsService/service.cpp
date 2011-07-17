/* WinBtrfsService/service.cpp
 * Windows service code
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

namespace WinBtrfsService
{
	SERVICE_STATUS status;
	SERVICE_STATUS_HANDLE hStatus;
	HANDLE stopEvent;

	void unmountEverything()
	{
		/* TODO: unmount all devices */
	}
	
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
				/* nothing to do while the service is running */
			}
			while (WaitForSingleObject(stopEvent, 10000) == WAIT_TIMEOUT);

			/* global cleanup tasks */
			CloseHandle(stopEvent);
			unmountEverything();

			status.dwControlsAccepted = 0;
			status.dwCurrentState = SERVICE_STOPPED;
			SetServiceStatus(hStatus, &status);
		}
	}
}

int main(int argc, char **argv)
{
	SERVICE_TABLE_ENTRY serviceTable[] =
	{
		{ L"WinBtrfsService", &WinBtrfsService::serviceMain },
		{ NULL, NULL }
	};

	/* this function will not return until all of this process's services have stopped */
	if (StartServiceCtrlDispatcher(serviceTable) == 0)
	{
		DWORD error = GetLastError();
		
		/* this would indicate a grave error on the part of the developer */
		assert(error != ERROR_INVALID_DATA);

		/* boneheaded user error */
		if (error == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT)
			fprintf(stderr, "Hey, you can't do that! This program must be run as a service!\n");

		return error;
	}
	else
		return 0;
}
