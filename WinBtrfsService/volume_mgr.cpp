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
#include "log.h"

namespace WinBtrfsService
{
	int mount(WinBtrfsDrv::MountData *mountData)
	{
		int error = 0;
		
		/* spawn a new instance of WinBtrfsLib */
		/* ensure that this FS UUID isn't already mounted somewhere else */

		log("mount: stub, returning %d\n", error);
		return error;
	}
	
	void unmountAll()
	{
		log("unmountAll is a stub!\n");
		// log on failure, this is nonfatal

		/* use DokanRemoveMountPoint(...); */
	}
}
