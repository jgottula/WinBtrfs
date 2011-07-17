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

#include <Windows.h>

namespace WinBtrfsService
{
	typedef VOID (WINAPI *LPSERVICE_MAIN_FUNCTIONW)(
	DWORD   dwNumServicesArgs,
	LPWSTR  *lpServiceArgVectors
	);

	void WINAPI serviceMain(DWORD dwNumServicesArgs, LPWSTR *lpServiceArgVectors)
	{

	}
}

int main(int argc, char **argv)
{
	SERVICE_TABLE_ENTRY serviceTable[2];

	serviceTable[0].lpServiceName = L"WinBtrfsService";
	serviceTable[0].lpServiceProc = &WinBtrfsService::serviceMain;

	serviceTable[1].lpServiceName = NULL;
	serviceTable[1].lpServiceProc = NULL;

	StartServiceCtrlDispatcher(serviceTable);

	return 0;
}
