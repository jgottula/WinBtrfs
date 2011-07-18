/* WinBtrfsDrv/WinBtrfsDrv.h
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

#ifndef WINBTRFSDRV_WINBTRFSDRV_H
#define WINBTRFSDRV_WINBTRFSDRV_H

#ifdef WINBTRFSDRV_EXPORTS
#define WINBTRFSDRV_API __declspec(dllexport)
#else
#define WINBTRFSDRV_API __declspec(dllimport)
#endif

#include "types.h"

namespace WinBtrfsDrv
{
	void WINBTRFSDRV_API start(const MountData *mountData);
	void WINBTRFSDRV_API terminate();
}

#endif
