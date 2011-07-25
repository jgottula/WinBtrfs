/* WinBtrfsService/ipc.h
 * inter-process communication
 *
 * WinBtrfs
 * Copyright (c) 2011 Justin Gottula
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#ifndef WINBTRFSSERVICE_IPC_H
#define WINBTRFSSERVICE_IPC_H

#include <Windows.h>

namespace WinBtrfsService
{
	/* these values MUST NOT change! */
	enum MsgType : int
	{
		MSG_REPLY_OK = 0,			/* indicates success
										no data */
		MSG_REPLY_OK_DATA = 1,		/* indicates success; also carries data
										data depends on the query */
		MSG_REPLY_ERROR = 2,		/* indicates an error
										data is an int representing an error code (may be MountError or other enum) */
		MSG_QUERY_MOUNT = 3,		/* from frontend: requests that a volume be mounted
										data should be a MountData struct (variably sized) */
		MSG_QUERY_DRV_START = 4,	/* from backend: requests information about the volume to mount
										no data */
		MSG_QUERY_FS_UUID = 5,		/* from backend: ensures that no volumes are mounted in multiple locations
										data should be a UUID (0x10 bytes) */
		MSG_QUERY_DRV_FAILURE = 6,	/* from backend: indicates a fatal problem; the volume entry should be removed
										no data */
	};

	enum MountError : int
	{
		MOUNT_ERROR_ALREADY_MOUNTED,
		MOUNT_ERROR_CREATE_PROCESS_FAILURE
	};
	
	struct ServiceMsg
	{
		MsgType type;
		size_t dataLen;
		char data[0];
	};

	DWORD setUpIPC();
	void cleanUpIPC();
	void handleIPC();
	DWORD WINAPI watchIPC(LPVOID lpParameter);
}

#endif
