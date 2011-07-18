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
#include <cstdio>
#include "../WinBtrfsLib/WinBtrfsLib.h"

using namespace WinBtrfsLib;

namespace WinBtrfsCLI
{
	BOOL WINAPI ctrlHandler(DWORD dwCtrlType)
	{
		printf("ctrlHandler: received 0x%x, terminating gracefully\n", dwCtrlType);

		WinBtrfsLib::terminate();

		/* here for completeness; should have terminated by this point */
		return FALSE;
	}

	void usage()
	{
		printf("Usage: WinBtrfsCLI.exe [options] <mount point> <device> [<device> ...]\n\n"
			"Options:\n"
			"--no-dump         don't dump trees at startup\n"
			"--dump-only       only dump trees, don't actually mount the volume\n"
			"--subvol=<name>   mount the subvolume with the given name\n"
			"--subvol-id=<ID>  mount the subvolume with the given object ID\n");

		exit(1);
	}

	void usageError(const char *format, ...)
	{
		va_list args;

		va_start(args, format);
		vprintf(format, args);
		va_end(args);

		usage();
	}

	void handleArgs(int argc, char **argv)
	{
		MountData *mountData = (MountData *)malloc(sizeof(MountData));
		int argState = 0;

		mountData->noDump = false;
		mountData->dumpOnly = false;
		mountData->useSubvolID = false;
		mountData->useSubvolName = false;
		mountData->numDevices = 0;

		for (int i = 1; i < argc; i++)
		{
			if (argv[i][0] == '-')
			{
				if (strcmp(argv[i], "--no-dump") == 0)
					mountData->noDump = true;
				else if (strcmp(argv[i], "--dump-only") == 0)
					mountData->dumpOnly = true;
				else if (strncmp(argv[i], "--subvol-id=", 12) == 0)
				{
					if (strlen(argv[i]) > 12)
					{
						if (!mountData->useSubvolID && !mountData->useSubvolName)
						{
							if (sscanf(argv[i] + 12, "%I64u ", &mountData->subvolID) == 1)
								mountData->useSubvolID = true;
							else
								usageError("You entered an indecipherable subvolume object ID!\n\n");
						}
						else
							usageError("You specified more than one subvolume to mount!\n\n");
					}
					else
						usageError("You didn't specify a subvolume object ID!\n\n");
				}
				else if (strncmp(argv[i], "--subvol=", 9) == 0)
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
							usageError("You specified more than one subvolume to mount!\n\n");
					}
					else
						usageError("You didn't specify a subvolume name!\n\n");
				}
				else
					usageError("'%s' is not a recognized command-line option!\n\n", argv[i]);
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
					mountData = (MountData *)realloc(mountData->devicePaths,
						sizeof(MountData) + (mountData->numDevices * sizeof(wchar_t) * MAX_PATH));
					printf("handleArgs: warning, wcscpy!\n");
					wcscpy(mountData->devicePaths[mountData->numDevices - 1], devicePath);
				}
			}

			assert(argState == 0 || argState == 1);
		}

		if (mountData->noDump && mountData->dumpOnly)
			usageError("You cannot specify both --no-dump and --dump-only on a single run!\n\n");

		if (argState == 0)
			usageError("You didn't specify a mount point or devices to load!\n\n");

		if (mountData->numDevices == 0)
			usageError("You didn't specify one or more devices to load!\n\n");

		/* in the future, find WinBtrfsService and communicate with it
			and let it deal with WinBtrfsLib directly */
		WinBtrfsLib::start(*mountData);

		free(mountData);
	}
}

using namespace WinBtrfsCLI;

int main(int argc, char **argv)
{
	LoadLibraryA("WinBtrfsLib.dll");

	/* ensure that Ctrl+C and other things will terminate the driver gracefully */
	SetConsoleCtrlHandler(&WinBtrfsCLI::ctrlHandler, TRUE);

	printf("WinBtrfsCLI (Transitional Version)\n"
		"Copyright (c) 2011 Justin Gottula\n\n"
		"WinBtrfs is under heavy development. If you encounter a bug, please go to\n"
		"http://github.com/jgottula/WinBtrfs and file an issue!\n\n");

	WinBtrfsCLI::handleArgs(argc, argv);

	return 0;
}
