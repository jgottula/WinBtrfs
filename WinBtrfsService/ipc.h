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

namespace WinBtrfsService
{
	DWORD setUpIPC();
	void cleanUpIPC();
	DWORD WINAPI watchIPC(LPVOID lpParameter);
	void handleIPC();
}
