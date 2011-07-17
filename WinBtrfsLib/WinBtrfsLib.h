/* WinBtrfsLib/WinBtrfsLib.h
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

#ifdef WINBTRFSLIB_EXPORTS
	#define WINBTRFSLIB_API __declspec(dllexport)
#else
	#define WINBTRFSLIB_API __declspec(dllimport)
#endif

#include <vector>
#include <Windows.h>
#include "types.h"

namespace WinBtrfsLib
{
	struct VolumeInfo
	{
		bool noDump, dumpOnly, useSubvolID, useSubvolName;
		BtrfsObjID subvolID;
		char *subvolName;
		wchar_t mountPoint[MAX_PATH];
		std::vector<const wchar_t *> devicePaths;
	};
	
	void WINBTRFSLIB_API start(VolumeInfo v);
	void WINBTRFSLIB_API terminate();
}
