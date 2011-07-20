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
#include <vector>
#include "log.h"

namespace WinBtrfsService
{
	std::vector<VolEntry> volumes;
	
	int mount(WinBtrfsDrv::MountData *mountData)
	{
		int error = 0;
		
		/* wait for the process to request a MountData struct */
		/* have it ensure that this FS UUID isn't already mounted somewhere else */
		
		VolEntry entry;
		entry.state = INST_PROC_STARTED;
		entry.mountData = mountData;

		const char env[] = "WinBtrfsService=1\0NamedPipe=\\\\.\\pipe\\WinBtrfsService\0";
		STARTUPINFO startUpInfo = { sizeof(STARTUPINFO) };
		if ((error = CreateProcess(L".\\WinBtrfsDrv.exe", NULL, NULL, NULL, FALSE,
			CREATE_NO_WINDOW, (LPVOID)env, NULL, &startUpInfo, &entry.procInfo)) = 0)
		{
			log("Could not start a new instance of WinBtrfsDrv.exe: %s", getErrorMessage(error));
			return error;
		}

		volumes.push_back(entry);

		log("mount: OK\n");
		return error;
	}
	
	void unmountAll()
	{
		log("unmountAll is a stub!\n");
		// log on failure, this is nonfatal

		/* iterate across all mounted volumes
			- send a message to trigger an unmount
			- after a timeout, kill the process
			- call DokanRemoveMountPoint to make sure */
	}
}
