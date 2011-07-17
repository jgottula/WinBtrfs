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

#include <Windows.h>
#include "../WinBtrfsLib/WinBtrfsLib.h"

int main(int argc, char **argv)
{
	LoadLibraryA("WinBtrfsLib.dll");
	
	WinBtrfsLib::start();
}

#if 0
/* WinBtrfsLib/main.cpp
 * transitional CLI interface
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
#include <vector>
#include <boost/detail/endian.hpp>
#include <Windows.h>
#include "btrfs_system.h"
#include "chunktree_parser.h"
#include "endian.h"
#include "fstree_parser.h"
#include "roottree_parser.h"

extern BtrfsObjID mountedSubvol;

bool useSubvolID = false, useSubvolName = false;
BtrfsObjID subvolID;
char *subvolName;
bool noDump = false, dumpOnly = false;

void terminate()
{
	cleanUp();
	DokanRemoveMountPoint(mountPoint); // DokanUnmount only allows drive letters
	exit(0);
}

BOOL WINAPI ctrlHandler(DWORD dwCtrlType)
{
	printf("ctrlHandler: received 0x%x, terminating gracefully\n", dwCtrlType);

	terminate();
	return FALSE;
}

void firstTasks()
{
	DWORD error;

#ifndef BOOST_LITTLE_ENDIAN
#pragma message("Warning: support for non-little-endian architectures is untested!")
	printf("firstTasks: warning: support for non-little-endian architectures is untested!\n");
#endif

	if ((error = init()) != 0)
	{
		printf("firstTasks: failed to get a handle on the partition! (GetLastError: %d)\n", error);

		exit(1);
	}

	if ((error = setupBigDokanLock()) != 0)
	{
		printf("firstTasks: failed to setup the Big Dokan Lock! (GetLastError: %d)\n", error);

		exit(1);
	}

	if ((error = loadSBs(!noDump)) != 0)
	{
		cleanUp();
		exit(1);
	}
	
	if (verifyDevices() != 0)
	{
		cleanUp();
		exit(1);
	}

	loadSBChunks(!noDump);

	if (!noDump) parseChunkTree(CTOP_DUMP_TREE);
	parseChunkTree(CTOP_LOAD);

	if (!noDump) parseRootTree(RTOP_DUMP_TREE, NULL, NULL);

	if (!noDump)
	{
		parseFSTree(OBJID_FS_TREE, FSOP_DUMP_TREE, NULL, NULL, NULL, NULL, NULL);
		parseRootTree(RTOP_DUMP_SUBVOLS, NULL, NULL);
	}

	if (dumpOnly)
	{
		cleanUp();
		exit(0);
	}
	
	/* aesthetic line break */
	if (!noDump)
		printf("\n");

	if (!useSubvolID && !useSubvolName)
	{
		int result;
		if ((result = parseRootTree(RTOP_DEFAULT_SUBVOL, NULL, NULL)) != 0)
		{
			printf("firstTasks: could not find the default subvolume!\n");
			cleanUp();
			exit(1);
		}
	}
	else if (useSubvolName)
	{
		if (strcmp(subvolName, "default") == 0)
			mountedSubvol = OBJID_FS_TREE;
		else
		{
			int result;
			if ((result = parseRootTree(RTOP_GET_SUBVOL_ID, subvolName, &mountedSubvol)) != 0)
			{
				printf("firstTasks: could not find the subvolume named '%s'!\n", subvolName);
				cleanUp();
				exit(1);
			}
		}
	}
	else
	{
		bool subvolExists;
		
		/* we can be fairly certain that these constraints will always hold */
		/* not enforcing these would allow the user to mount non-FS-type trees, which is definitely bad */
		if ((subvolID > (BtrfsObjID)0 && subvolID < (BtrfsObjID)0x100) ||
			(subvolID > (BtrfsObjID)-0x100 && subvolID < (BtrfsObjID)-1))
		{
			printf("firstTasks: %I64u is an impossible subvolume ID!\n", (unsigned __int64)subvolID);
			cleanUp();
			exit(1);
		}
		
		parseRootTree(RTOP_SUBVOL_EXISTS, &subvolID, &subvolExists);

		if (!subvolExists)
		{
			printf("firstTasks: could not find the subvolume with ID %I64u!\n", (unsigned __int64)subvolID);
			cleanUp();
			exit(1);
		}

		mountedSubvol = (subvolID == (BtrfsObjID)0 ? OBJID_FS_TREE : subvolID);
	}
}

void dokanError(int dokanResult)
{
	switch (dokanResult)
	{
	case DOKAN_SUCCESS:
		printf("Dokan terminated successfully.\n");
		break;
	case DOKAN_ERROR:
		printf("Dokan reported a general error!\n");
		exit(1);
	case DOKAN_DRIVE_LETTER_ERROR:
		printf("Dokan reported a bad drive letter!\n");
		exit(1);
	case DOKAN_DRIVER_INSTALL_ERROR:
		printf("Dokan reported it couldn't install the driver!\n");
		exit(1);
	case DOKAN_START_ERROR:
		printf("Dokan reported something is wrong with the driver!\n");
		exit(1);
	case DOKAN_MOUNT_ERROR:
		printf("Dokan reported it couldn't assign a drive letter or mount point!\n"
			"Sometimes this problem is resolved by running WinBtrfsCLI again.\n");
		exit(1);
	case DOKAN_MOUNT_POINT_ERROR:
		printf("Dokan reported the mount point is invalid!\n");
		exit(1);
	default:
		printf("Dokan returned an unknown error!\n");
		exit(1);
	}
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

/* this function (and probably the entirety of this function needs to be moved to WinBtrfsCLI.
	in the fucture, WinBtrfsCLI will validate user CLI input, then call DLL interfaces to start,
	terminate, (and eventually control) WinBtrfsLib. */
__declspec(dllexport) int __stdcall winBtrfsLib_main(int argc, char **argv)
{
	PDOKAN_OPTIONS dokanOptions;
	int argState = 0, dokanResult;

	printf("WinBtrfsLib Transitional CLI\n"
		"Copyright (c) 2011 Justin Gottula\n\n"
		"WinBtrfs is under heavy development. If you encounter a bug, please go to\n"
		"http://github.com/jgottula/WinBtrfs and file an issue!\n\n");

	for (int i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			if (strcmp(argv[i], "--no-dump") == 0)
				noDump = true;
			else if (strcmp(argv[i], "--dump-only") == 0)
				dumpOnly = true;
			else if (strncmp(argv[i], "--subvol-id=", 12) == 0)
			{
				if (strlen(argv[i]) > 12)
				{
					if (!useSubvolID && !useSubvolName)
					{
						if (sscanf(argv[i] + 12, "%I64u ", &subvolID) == 1)
							useSubvolID = true;
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
					if (!useSubvolID && !useSubvolName)
					{
						subvolName = argv[i] + 9;
						useSubvolName = true;
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
				assert(mbstowcs_s(NULL, mountPoint, MAX_PATH, argv[i], strlen(argv[i])) == 0);

				argState++;
			}
			else
			{
				wchar_t *devicePath = new wchar_t[MAX_PATH];

				/* isn't fatal, shouldn't be an assertion really */
				assert(mbstowcs_s(NULL, devicePath, MAX_PATH, argv[i], strlen(argv[i])) == 0);

				devicePaths.push_back(devicePath);
			}
		}

		assert(argState == 0 || argState == 1);
	}

	if (noDump && dumpOnly)
		usageError("You cannot specify both --no-dump and --dump-only on a single run!\n\n");

	if (argState == 0)
		usageError("You didn't specify a mount point or devices to load!\n\n");

	if (devicePaths.size() == 0)
		usageError("You didn't specify one or more devices to load!\n\n");

	dokanOptions = (PDOKAN_OPTIONS)malloc(sizeof(DOKAN_OPTIONS));
	dokanOptions->Version = 600;
	dokanOptions->ThreadCount = 1;			// eventually set this to zero or a user-definable count
	dokanOptions->Options = 0;				// look into this later
	dokanOptions->GlobalContext = 0;		// use this later if necessary
	dokanOptions->MountPoint = mountPoint;

	firstTasks();

	/* ensure that Ctrl+C and other things will terminate the driver gracefully */
	SetConsoleCtrlHandler(&ctrlHandler, TRUE);

	dokanResult = DokanMain(dokanOptions, &btrfsOperations);

	cleanUp();
	dokanError(dokanResult);
	
	return 0;
}
#endif
