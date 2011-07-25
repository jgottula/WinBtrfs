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

#include "ipc.h"
#include "../WinBtrfsDrv/types.h"

using namespace WinBtrfsDrv;

namespace WinBtrfsService
{
	enum InstState
	{
		INST_DEAD,
		INST_PROC_STARTED,
		INST_MOUNTED
	};
	
	struct VolEntry
	{
		InstState state;
		PROCESS_INFORMATION procInfo;
		WinBtrfsDrv::MountData *mountData;
	};
	
	bool mount(MountData *mountData, MountError *mError);
	void unmountAll();
}

#endif