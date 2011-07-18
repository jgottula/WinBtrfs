/* WinBtrfsService/volume_mgr.h
 * manages all currently mounted volumes
 *
 * WinBtrfs
 * Copyright (c) 2011 Justin Gottula
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#ifndef WINBTRFSSERVICE_VOLUME_MGR_H
#define WINBTRFSSERVICE_VOLUME_MGR_H

#include "../WinBtrfsLib/WinBtrfsLib.h"
#include "WinBtrfsService.h"

namespace WinBtrfsService
{
	int mount(WinBtrfsLib::MountData *mountData);
	void unmountAll();
}

#endif
