/* WinBtrfsDrv/init.h
 * filesystem initialization code
 *
 * WinBtrfs
 * Copyright (c) 2011 Justin Gottula
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#ifndef WINBTRFSDRV_INIT_H
#define WINBTRFSDRV_INIT_H

#include "types.h"

namespace WinBtrfsDrv
{
	extern MountData *mountData;
	
	void init();
}

#endif
