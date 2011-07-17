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
#include <Windows.h>

namespace WinBtrfsService
{
	DWORD WINAPI serviceCtrlHandler(DWORD dwControl, DWORD dwEventType,
		LPVOID lpEventData, LPVOID lpContext)
	{
		switch (dwControl)
		{
		default:
			return ERROR_CALL_NOT_IMPLEMENTED;
		}
	}
	
	void WINAPI serviceMain(DWORD dwNumServicesArgs, LPWSTR *lpServiceArgVectors)
	{
		SERVICE_STATUS status;

		status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
		status.dwCurrentState = SERVICE_STOPPED;
		status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
		status.dwWin32ExitCode = NO_ERROR;
		status.dwServiceSpecificExitCode = NO_ERROR;
		status.dwCheckPoint = 0;
		status.dwWaitHint = 0;

		SERVICE_STATUS_HANDLE hStatus = RegisterServiceCtrlHandlerEx(L"WinBtrfsService",
			&serviceCtrlHandler, NULL);
		assert(hStatus != 0);
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
	return StartServiceCtrlDispatcher(serviceTable);
}
