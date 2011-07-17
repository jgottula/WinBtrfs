/* WinBtrfsService/volume_mgr.cpp
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

#include "volume_mgr.h"
#include "../WinBtrfsLib/WinBtrfsLib.h"
#include "log.h"

namespace WinBtrfsService
{
	void mount()
	{
		log("mount is a stub!\n");
		// log on failure, this is nonfatal
	}
	
	void unmountAll()
	{
		log("unmountAll is a stub!\n");
		// log on failure, this is nonfatal
	}
}
