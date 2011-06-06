/* main.c
 * CLI processing and Dokan stuff
 *
 * WinBtrfs
 *
 * Copyright (c) 2011 Justin Gottula
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <stdio.h>
#include <assert.h>
#include <Windows.h>
#include <dokan.h>
#include "structures.h"
#include "endian.h"
#include "btrfs_system.h"
#include "dokan_callbacks.h"

WCHAR devicePath[MAX_PATH], mountPoint[MAX_PATH];
DOKAN_OPERATIONS btrfsOperations = {
	&btrfsCreateFile,
	&btrfsOpenDirectory,
	&btrfsCreateDirectory,
	&btrfsCleanup,
	&btrfsCloseFile,
	&btrfsReadFile,
	&btrfsWriteFile,
	&btrfsFlushFileBuffers,
	&btrfsGetFileInformation,
	&btrfsFindFiles,
	NULL, // FindFilesWithPattern
	&btrfsSetFileAttributes,
	&btrfsSetFileTime,
	&btrfsDeleteFile,
	&btrfsDeleteDirectory,
	&btrfsMoveFile,
	&btrfsSetEndOfFile,
	&btrfsSetAllocationSize,
	&btrfsLockFile,
	&btrfsUnlockFile,
	&btrfsGetDiskFreeSpace,
	&btrfsGetVolumeInformation,
	&btrfsUnmount,
	&btrfsGetFileSecurity,
	&btrfsSetFileSecurity
};

void firstTasks()
{
	DWORD errorCode;
	int secSB;

	endianDetect();

	if ((errorCode = init()) != 0)
	{
		printf("Failed to get a handle on the partition! (GetLastError: %d)\n", errorCode);

		exit(1);
	}

	if ((errorCode = readPrimarySB()) != 0)
	{
		printf("Failed to read the primary superblock! (GetLastError: %d)\n", errorCode);

		cleanUp();
		exit(1);
	}

	switch (validateSB(NULL))
	{
	case 0:
		printf("Primary superblock is OK.\n");
		break;
	case 1:
		printf("Superblock is missing or invalid!\n");
		cleanUp();
		exit(1);
	case 2:
		printf("Superblock checksum failed!\n");
		cleanUp();
		exit(1);
	default:
		printf("Superblock failed to validate for an unknown reason!\n");
		cleanUp();
		exit(1);
	}

	if ((secSB = findSecondarySBs()) != 1)
		printf("Found a newer secondary superblock (#%d).\n", secSB);

	parseChunkTree();
	parseRootTree();
	dump();
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
		printf("Dokan reported it couldn't assign a drive letter or mount point!\n");
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
	printf("Usage: WinBtrfsCLI.exe <device> <mount point>\n\n"
		"For the device argument, try something like \\Device\\HarddiskX\\PartitionY.\n"
		"Disks are indexed from zero; partitions are indexed from one.\n"
		"Example: /dev/sda1 = \\\\.\\Harddisk0Partition1\n\n"
		"You can also specify an image file to mount.\n\n"
		"The mount point can be a drive letter or an empty NTFS directory.\n");

	exit(1);
}

void unitTests()
{
	/* verify sizes of important on-disk structures */
	assert(sizeof(BtrfsTime) == 0x0c);
	assert(sizeof(BtrfsHeader) == 0x65);
	assert(sizeof(BtrfsDiskKey) == 0x11);
	assert(sizeof(BtrfsKeyPtr) == 0x21);
	assert(sizeof(BtrfsItem) == 0x19);
	assert(sizeof(BtrfsInodeItem) == 0xa0);
	assert(sizeof(BtrfsInodeRef) == 0x14);
	assert(sizeof(BtrfsDirItem) == 0x1e);
	assert(sizeof(BtrfsDirIndex) == 0x1e);
	assert(sizeof(BtrfsExtentData) == 0x15);
	assert(sizeof(BtrfsExtentDataNonInline) == 0x20);
	assert(sizeof(BtrfsRootItem) == 0xef);
	assert(sizeof(BtrfsBlockGroupItem) == 0x18);
	assert(sizeof(BtrfsChunkItem) == 0x30);
	assert(sizeof(BtrfsChunkItemStripe) == 0x20);
	assert(sizeof(BtrfsDevItem) == 0x62);
	assert(sizeof(BtrfsSuperblock) == 0x1000);
}

int main(int argc, char **argv)
{
	PDOKAN_OPTIONS dokanOptions;
	int dokanResult;

	printf("WinBtrfsCLI\nCopyright (c) 2011 Justin Gottula\n\n");

	unitTests();

	if (argc != 3)
		usage();

	/* Need argument validity checking, MAX_PATH checking for buffer overruns */
	
	mbstowcs_s(NULL, devicePath, MAX_PATH, argv[1], strlen(argv[1]));
	mbstowcs_s(NULL, mountPoint, MAX_PATH, argv[2], strlen(argv[2]));

	dokanOptions = (PDOKAN_OPTIONS)malloc(sizeof(DOKAN_OPTIONS));
	dokanOptions->Version = 600;
	dokanOptions->ThreadCount = 1;			// eventually set this to zero or a user-definable count
	dokanOptions->Options = 0;				// look into this later
	dokanOptions->GlobalContext = 0;		// use this later if necessary
	dokanOptions->MountPoint = mountPoint;

	firstTasks();

	dokanResult = DokanMain(dokanOptions, &btrfsOperations);

	cleanUp();
	dokanError(dokanResult);
	
	return 0;
}
