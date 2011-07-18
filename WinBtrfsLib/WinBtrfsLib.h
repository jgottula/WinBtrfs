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

#ifndef WINBTRFSLIB_WINBTRFSLIB_H
#define WINBTRFSLIB_WINBTRFSLIB_H

#ifdef WINBTRFSLIB_EXPORTS
#define WINBTRFSLIB_API __declspec(dllexport)
#else
#define WINBTRFSLIB_API __declspec(dllimport)
#endif

#include "types.h"

namespace WinBtrfsLib
{
	void WINBTRFSLIB_API start(MountData mountData);
	void WINBTRFSLIB_API terminate();
}

#endif
