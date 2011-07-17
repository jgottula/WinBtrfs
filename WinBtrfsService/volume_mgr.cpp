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
	int mount(MountData *mountData)
	{
		char *logMsg = (char *)malloc(10240);

		sprintf(logMsg, "mount: mountData contains the following:\n"
			"noDump: %s\ndumpOnly: %s\nuseSubvolID: %s\nuseSubvolName: %s\n"
			"subvolID: %I64u\nsubvolName: %s\nmountPoint: %S\nnumDevices: %u\n",
			(mountData->noDump ? "true" : "false"),
			(mountData->dumpOnly ? "true" : "false"),
			(mountData->useSubvolID ? "true" : "false"),
			(mountData->useSubvolName ? "true" : "false"),
			(unsigned __int64)mountData->subvolID,
			mountData->subvolName, mountData->mountPoint, mountData->numDevices);

		for (size_t i = 0; i < mountData->numDevices; i++)
			sprintf(logMsg + strlen(logMsg), "devicePaths[%u]: %S\n",
				i, (mountData->devicePaths + (MAX_PATH * i)));

		log(logMsg);

		/* spawn a new process to contain its own instance of WinBtrfsLib */
		/* ensure that this FS UUID isn't already mounted somewhere else */
		/* log on failure */

		free(logMsg);

		return 0;
	}
	
	void unmountAll()
	{
		log("unmountAll is a stub!\n");
		// log on failure, this is nonfatal
	}
}
