/* WinBtrfsLib/instance.cpp
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

namespace WinBtrfsLib
{
	std::map<DWORD, InstanceData *> instances;
	
	InstanceData *getThisInst()
	{
		DWORD thID = GetCurrentThreadId();
		
		/* ensure that a structure for this instance exists in the array */
		assert(instances.find(thID) == instances.end());

		return instances[thID];
	}
}
