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
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	
	DWORD setUpIPC()
	{
		if ((hPipe = CreateNamedPipe(L"\\\\.\\pipe\\WinBtrfsService", PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_NOWAIT, PIPE_UNLIMITED_INSTANCES,
			1024, 1024, 0, NULL)) == INVALID_HANDLE_VALUE)
		{
			DWORD error = GetLastError();
			
			log("CreateNamedPipe returned error %u: %s", error, getErrorMessage(error));
			
			return error;
		}

		return 0;
	}

	void cleanUpIPC()
	{
		CloseHandle(hPipe);
	}
	
	void checkIPC()
	{
		// log on failure, this is nonfatal
	}

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
