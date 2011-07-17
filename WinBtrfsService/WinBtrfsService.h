/* WinBtrfsService/WinBrtfsService.h
 * public structures
 *
 * WinBtrfs
 * Copyright (c) 2011 Justin Gottula
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#ifndef WINBTRFSSERVICE_WINBTRFSSERVICE_H
#define WINBTRFSSERVICE_WINBTRFSSERVICE_H

#include <vector>
#include <Windows.h>
#include "../WinBtrfsLib/types.h"

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

	struct MountData
	{
		bool noDump, dumpOnly, useSubvolID, useSubvolName;
		WinBtrfsLib::BtrfsObjID subvolID;
		char *subvolName;
		wchar_t mountPoint[MAX_PATH];
		std::vector<const wchar_t *> devicePaths;
	};
}

#endif
