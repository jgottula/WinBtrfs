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

#include "WinBtrfsService.h"

namespace WinBtrfsService
{
	int mount(MountData *mountData);
	void unmountAll();
}
