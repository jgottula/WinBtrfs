/* WinBtrfsLib/WinBtrfsLib.cpp
 * public DLL interfaces
 *
 * WinBtrfs
 * Copyright (c) 2011 Justin Gottula
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "WinBtrfsLib.h"
#include <cstdio>
#include <vector>
#include <Windows.h>
#include "btrfs_system.h"
#include "dokan_callbacks.h"

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

wchar_t mountPoint[MAX_PATH];
std::vector<const wchar_t *> devicePaths;

BOOL WINAPI DLLMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	return TRUE;
}

namespace WinBtrfsLib
{
	void WINBTRFSLIB_API start()
	{

	}

	void WINBTRFSLIB_API terminate()
	{
		cleanUp();
		DokanRemoveMountPoint(mountPoint); // DokanUnmount only allows drive letters
		exit(0);
	}
}
