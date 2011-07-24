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
	enum MsgType
	{
		MSG_QUERY_MOUNT,		/* from frontend: requests that a volume be mounted
									data should be a MountData struct (variably sized) */
		MSG_QUERY_DRV_START,	/* from backend: requests information about the volume to mount
									no data */
		MSG_QUERY_FS_UUID,		/* from backend: ensures that no volumes are mounted in multiple locations
									data should be a UUID (0x10 bytes) */
		MSG_QUERY_DRV_FAILURE,	/* from backend: indicates a fatal problem; the volume entry should be removed
									no data */
		MSG_REPLY_OK,			/* indicates success
									no data */
		MSG_REPLY_OK_DATA,		/* indicates success; also carries data
									data depends on the query */
		MSG_REPLY_ERROR			/* indicates an error
									data is an int representing an error code */
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
