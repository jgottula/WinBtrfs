/* PipeClient/main.cpp
 * named pipe client for testing purposes
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
#include "../WinBtrfsDrv/types.h"
#include "../WinBtrfsService/ipc.h"

/* based on http://ist.marshall.edu/ist480acp/code/pipec.cpp */

using namespace WinBtrfsDrv;
using namespace WinBtrfsService;

int main(int argc, char **argv)
{
	HANDLE hPipe = INVALID_HANDLE_VALUE;

	if (WaitNamedPipe(L"\\\\.\\pipe\\WinBtrfsService", 1000) == 0)
	{
		fprintf(stderr, "WaitNamedPipe failed: %u\n", GetLastError());
		return 1;
	}
	
	printf("Found a server on the name pipe.\n");

	if ((hPipe = CreateFile(L"\\\\.\\pipe\\WinBtrfsService", GENERIC_READ | GENERIC_WRITE,
		0, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "CreateFile failed: %u\n", GetLastError());
		return 2;
	}

	printf("Opened the pipe successfully.\n");

	const size_t DATA_LEN = sizeof(MountData) + (2 * MAX_PATH * sizeof(wchar_t));
	ServiceMsg *msg = (ServiceMsg *)malloc(sizeof(ServiceMsg) + DATA_LEN);
	MountData *mountData = (MountData *)msg->data;
	DWORD bytesWritten = 0;

	mountData->dumpOnly = false;
	mountData->noDump = false;
	mountData->useSubvolID = true;
	mountData->useSubvolName = false;
	mountData->subvolID = (BtrfsObjID)256;
	mountData->subvolName[0] = 0;
	wcscpy(mountData->mountPoint, L"X:");
	mountData->numDevices = 2;
	wcscpy(mountData->devicePaths[0], L"..\\..\\test_images\\btrfs_multiA.img");
	wcscpy(mountData->devicePaths[1], L"..\\..\\test_images\\btrfs_multiB.img");

	msg->type = MSG_QUERY_MOUNT;
	msg->dataLen = DATA_LEN;

	if (WriteFile(hPipe, msg, sizeof(ServiceMsg) + DATA_LEN, &bytesWritten, NULL) == 0)
	{
		fprintf(stderr, "WriteFile failed: %u\n", GetLastError());
		CloseHandle(hPipe);
		return 3;
	}

	free(msg);

	printf("Sent REQ_MOUNT.\n", msg);

	msg = (ServiceMsg *)malloc(sizeof(ServiceMsg) + sizeof(int));
	DWORD bytesRead = 0;

	if (ReadFile(hPipe, msg, sizeof(ServiceMsg) + sizeof(int), &bytesRead, NULL) == 0)
	{
		fprintf(stderr, "ReadFile failed: %u\n", GetLastError());
		CloseHandle(hPipe);
		return 4;
	}

	if (msg->type == MSG_REPLY_OK)
		printf("Received MSG_REPLY_OK.\n");
	else if (msg->type == MSG_REPLY_ERROR)
		printf("Received MSG_REPLY_ERROR (%d).\n", *((int *)msg->data));

	free(msg);

	CloseHandle(hPipe);

	printf("Closed the connection.\n");

	return 0;
}
