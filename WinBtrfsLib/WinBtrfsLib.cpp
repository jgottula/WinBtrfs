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
#include <dokan.h>
#include "btrfs_system.h"
#include "init.h"

namespace WinBtrfsLib
{
	extern std::vector<const wchar_t *> *devicePaths;
	
	VolumeInfo volumeInfo;

	void WINBTRFSLIB_API start(VolumeInfo v)
	{
		memcpy(&volumeInfo, &v, sizeof(VolumeInfo));
		devicePaths = &volumeInfo.devicePaths;

		init();
	}

	void WINBTRFSLIB_API terminate()
	{
		cleanUp();
		DokanRemoveMountPoint(volumeInfo.mountPoint); // DokanUnmount only allows drive letters
		exit(0);
	}
}

BOOL WINAPI DLLMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	return TRUE;
}
