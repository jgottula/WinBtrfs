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

/* based on http://ist.marshall.edu/ist480acp/code/pipec.cpp */

const char msg[] = "Hello from PipeClient!";

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

	DWORD bytesWritten = 0;

	if (WriteFile(hPipe, msg, strlen(msg) + 1, &bytesWritten, NULL) == 0)
	{
		fprintf(stderr, "WriteFile failed: %u\n", GetLastError());
		CloseHandle(hPipe);
		return 3;
	}

	printf("Sent message: '%s'.\n", msg);

	char buffer[1024];
	DWORD bytesRead = 0;

	if (ReadFile(hPipe, buffer, 1024, &bytesRead, NULL) == 0)
	{
		fprintf(stderr, "ReadFile failed: %u\n", GetLastError());
		CloseHandle(hPipe);
		return 4;
	}

	printf("Received message: '%s'.\n", buffer);

	CloseHandle(hPipe);

	printf("Closed the connection.\n");

	return 0;
}
