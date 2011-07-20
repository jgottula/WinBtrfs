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

#include <Windows.h>

#ifndef WINBTRFSSERVICE_IPC_H
#define WINBTRFSSERVICE_IPC_H

namespace WinBtrfsService
{
	enum MsgType
	{
		MSG_RESP_OK = 0,
		MSG_RESP_ERROR = 1,
		MSG_REQ_MOUNT = 2
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
