/* WinBtrfsDrv/instance.cpp
 * infrastructure to handle multiple instances
 *
 * WinBtrfs
 * Copyright (c) 2011 Justin Gottula
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "instance.h"
#include <cassert>

namespace WinBtrfsDrv
{
	std::map<DWORD, InstanceData *> instances;
	
	InstanceData *getThisInst()
	{
		DWORD thID = GetCurrentThreadId();
		
		/* ensure that a structure for this instance exists in the array */
		assert(instances.count(thID) > 0);

		return instances[thID];
	}
	
	InstanceData *getInstByMountPoint(const wchar_t *mountPoint)
	{
		std::map<DWORD, InstanceData *>::iterator it = instances.begin(),
			end = instances.end();
		for ( ; it != end; ++it)
		{
			InstanceData *inst = it->second;

			if (wcscmp(inst->mountData->mountPoint, mountPoint) == 0)
				return inst;
		}

		/* if control gets here, we failed to find an instance for the given mount point */
		assert(0);
	}
}
