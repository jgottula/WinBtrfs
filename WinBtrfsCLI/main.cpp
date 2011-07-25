/* WinBtrfsCLI/main.cpp
 * CLI interface
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
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include "../WinBtrfsDrv/types.h"
/*#include "../WinBtrfsService/ipc.h"*/

using namespace WinBtrfsDrv;
/*using namespace WinBtrfsService;*/

namespace WinBtrfsCLI
{
	void usage()
	{
		puts("\n\nUsage: WinBtrfsCLI.exe <command> [parameters]\n\n"
			"Commands: mount, [more to be added]\n\n"
			"WinBtrfsCLI.exe mount [options] <mount point> <device> [<device> ...]\n"
			"Options:\n"
			"--no-dump         don't dump trees at startup\n"
			"--dump-only       only dump trees, don't actually mount the volume\n"
			"--subvol=<name>   mount the subvolume with the given name (case sensitive)\n"
			"--subvol-id=<ID>  mount the subvolume with the given object ID\n");

		exit(1);
	}

	void usageError(const char *format, ...)
	{
		va_list args;

		va_start(args, format);
		vfprintf(stderr, format, args);
		va_end(args);

		usage();
	}

	void pipeError(int rtn, const char *format, ...)
	{
		va_list args;

		fputs("Failed to communicate with WinBtrfsService!\n", stderr);

		va_start(args, format);
		vfprintf(stderr, format, args);
		va_end(args);

		exit(rtn);
	}

	void sendMountMsg(MountData *mountData)
	{
		HANDLE hPipe = INVALID_HANDLE_VALUE;

		if (WaitNamedPipe(L"\\\\.\\pipe\\WinBtrfsService", 1000) == 0)
			pipeError(GetLastError(), "WaitNamedPipe failed: %u\n", GetLastError());

		if ((hPipe = CreateFile(L"\\\\.\\pipe\\WinBtrfsService", GENERIC_READ | GENERIC_WRITE,
			0, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
			pipeError(GetLastError(), "CreateFile failed: %u\n", GetLastError());
		
		/* remove me */
		DWORD bytesWritten = 0;
		WriteFile(hPipe, "Hello!", 7, &bytesWritten, NULL);

#if 0
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
#endif
	}
	
	void handleArgs(int argc, char **argv)
	{
		if (argc <= 1)
			usageError("You didn't specify a command!\n");

		if (stricmp(argv[1], "mount") == 0)
		{
			MountData *mountData = (MountData *)malloc(sizeof(MountData));
			int argState = 0;

			mountData->noDump = false;
			mountData->dumpOnly = false;
			mountData->useSubvolID = false;
			mountData->useSubvolName = false;
			mountData->subvolID = (BtrfsObjID)256;
			mountData->subvolName[0] = 0;
			mountData->numDevices = 0;

			for (int i = 2; i < argc; i++)
			{
				if (argv[i][0] == '-')
				{
					if (stricmp(argv[i], "--no-dump") == 0)
						mountData->noDump = true;
					else if (stricmp(argv[i], "--dump-only") == 0)
						mountData->dumpOnly = true;
					else if (strnicmp(argv[i], "--subvol-id=", 12) == 0)
					{
						if (strlen(argv[i]) > 12)
						{
							if (!mountData->useSubvolID && !mountData->useSubvolName)
							{
								if (sscanf(argv[i] + 12, "%I64u ", &mountData->subvolID) == 1)
									mountData->useSubvolID = true;
								else
									usageError("You entered an indecipherable subvolume object ID!\n");
							}
							else
								usageError("You specified more than one subvolume to mount!\n");
						}
						else
							usageError("You didn't specify a subvolume object ID!\n");
					}
					else if (strnicmp(argv[i], "--subvol=", 9) == 0)
					{
						if (strlen(argv[i]) > 9)
						{
							if (!mountData->useSubvolID && !mountData->useSubvolName)
							{
								printf("handleArgs: warning, strcpy!\n");
								strcpy(mountData->subvolName, argv[i] + 9);
								mountData->useSubvolName = true;
							}
							else
								usageError("You specified more than one subvolume to mount!\n");
						}
						else
							usageError("You didn't specify a subvolume name!\n");
					}
					else
						usageError("'%s' is not a recognized command-line option!\n", argv[i]);
				}
				else
				{
					if (argState == 0)
					{
						/* isn't fatal, shouldn't be an assertion really */
						assert(mbstowcs_s(NULL, mountData->mountPoint, MAX_PATH, argv[i], strlen(argv[i])) == 0);

						argState++;
					}
					else
					{
						wchar_t *devicePath = new wchar_t[MAX_PATH];

						/* isn't fatal, shouldn't be an assertion really */
						assert(mbstowcs_s(NULL, devicePath, MAX_PATH, argv[i], strlen(argv[i])) == 0);

						mountData->numDevices++;
						mountData = (MountData *)realloc(mountData, sizeof(MountData) +
							(mountData->numDevices * sizeof(wchar_t) * MAX_PATH));
						printf("handleArgs: warning, wcscpy!\n");
						wcscpy(mountData->devicePaths[mountData->numDevices - 1], devicePath);
					}
				}

				assert(argState == 0 || argState == 1);
			}

			if (mountData->noDump && mountData->dumpOnly)
				usageError("You cannot specify both --no-dump and --dump-only on a single run!\n");

			if (argState == 0)
				usageError("You didn't specify a mount point or devices to load!\n");

			if (mountData->numDevices == 0)
				usageError("You didn't specify one or more devices to load!\n");

			sendMountMsg(mountData);
			free(mountData);
		}
		else
			usageError("%s is not a recognized command!\n", argv[1]);
	}
}

using namespace WinBtrfsCLI;

int main(int argc, char **argv)
{
	printf("WinBtrfsCLI (Post-Transitional Version)\n"
		"Copyright (c) 2011 Justin Gottula\n\n"
		"WinBtrfs is under heavy development. If you encounter a bug, please go to\n"
		"http://github.com/jgottula/WinBtrfs and file an issue!\n\n");

	WinBtrfsCLI::handleArgs(argc, argv);

	return 0;
}
