/* WinBtrfsDrv/util.cpp
 * utility functions
 *
 * WinBtrfs
 * Copyright (c) 2011 Justin Gottula
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "util.h"
#include <cassert>
#include <cstdio>
#include <vector>
#include <dokan.h>
#include "btrfs_system.h"
#include "constants.h"
#include "endian.h"

namespace WinBtrfsDrv
{
	void convertTime(const BtrfsTime *bTime, PFILETIME wTime)
	{
		LONGLONG s64 = 116444736000000000; // 1601-to-1970 correction factor

		s64 += endian64(bTime->secSince1970) * 10000000;
		s64 += endian32(bTime->nanoseconds) / 100;

		wTime->dwHighDateTime = (DWORD)(s64 >> 32);
		wTime->dwLowDateTime = (DWORD)s64;
	}

	void convertMetadata(const FilePkg *input, void *output, bool dirList)
	{
		LPBY_HANDLE_FILE_INFORMATION fileInfo = (LPBY_HANDLE_FILE_INFORMATION)output;
		PWIN32_FIND_DATAW dirListData = (PWIN32_FIND_DATAW)output;

		/* FILE_ATTRIBUTE_COMPRESSED, FILE_ATTRIBUTE_SPARSE_FILE, FILE_ATTRIBUTE_REPARSE_POINT (maybe) */
		printf("convertMetadata: TODO: handle more attributes\n");

		if (!dirList)
		{
			fileInfo->dwFileAttributes = 0;
			if (endian32(input->inode.stMode) & S_IFBLK) fileInfo->dwFileAttributes |= FILE_ATTRIBUTE_DEVICE; // is this right?
			if (endian32(input->inode.stMode) & S_IFDIR) fileInfo->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
			if (!(endian32(input->inode.stMode) & S_IWUSR)) fileInfo->dwFileAttributes |= FILE_ATTRIBUTE_READONLY; // using owner perms
			if (input->hidden) fileInfo->dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN;
		}
		else
		{
			dirListData->dwFileAttributes = 0;
			if (endian32(input->inode.stMode) & S_IFBLK) dirListData->dwFileAttributes |= FILE_ATTRIBUTE_DEVICE; // is this right?
			if (endian32(input->inode.stMode) & S_IFDIR) dirListData->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
			if (input->hidden) dirListData->dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN;
		}
	
		/* not sure if this is necessary, but it seems to be what you're supposed to do */
		if (!dirList)
		{
			if (fileInfo->dwFileAttributes == 0)
				fileInfo->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
		}
		else
		{
			if (dirListData->dwFileAttributes == 0)
				dirListData->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
		}

		if (!dirList)
		{
			convertTime(&input->inode.stCTime, &fileInfo->ftCreationTime);
			convertTime(&input->inode.stATime, &fileInfo->ftLastAccessTime);
			convertTime(&input->inode.stMTime, &fileInfo->ftLastWriteTime);
		}
		else
		{
			convertTime(&input->inode.stCTime, &dirListData->ftCreationTime);
			convertTime(&input->inode.stATime, &dirListData->ftLastAccessTime);
			convertTime(&input->inode.stMTime, &dirListData->ftLastWriteTime);
		}

		if (!dirList)
		{
			/* using the least significant 4 bytes of the UUID */
			fileInfo->dwVolumeSerialNumber = supers[0].fsUUID[0] +
				(supers[0].fsUUID[1] << 8) +
				(supers[0].fsUUID[2] << 16) +
				(supers[0].fsUUID[3] << 24);
		}

		if (!dirList)
		{
			fileInfo->nFileSizeHigh = (DWORD)(endian64(input->inode.stSize) >> 32);
			fileInfo->nFileSizeLow = (DWORD)endian64(input->inode.stSize);
		}
		else
		{
			dirListData->nFileSizeHigh = (DWORD)(endian64(input->inode.stSize) >> 32);
			dirListData->nFileSizeLow = (DWORD)endian64(input->inode.stSize);
		}

		if (!dirList)
		{
			fileInfo->nNumberOfLinks = endian32(input->inode.stNLink);

			/* reimplement the file index values so they are unique values among the currently-open[/cleanedup] files.
				don't know quite how to do this yet, but treeID+objectID is 128 bits, definitely will not work for a 64-bit value. */
			printf("convertMetadata: TODO: properly set file index values\n");
			fileInfo->nFileIndexHigh = 0/*(DWORD)(endian64(it->objectID) << 32)*/;
			fileInfo->nFileIndexLow = 0/*(DWORD)endian64(it->objectID)*/;
		}

		if (dirList)
		{
			wchar_t nameW[MAX_PATH];
		
			size_t result = mbstowcs(nameW, input->name, MAX_PATH);
			assert(result == strlen(input->name));

			wcscpy(dirListData->cFileName, nameW);
			dirListData->cAlternateFileName[0] = 0; // no 8.3 name
		}
	}

	void hexToChar(unsigned char hex, char *chr)
	{
		static const char digits[] = { '0', '1', '2', '3', '4', '5', '6', '7',
			'8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

		*chr = digits[hex & 0x0f];
	}

	void uuidToStr(const unsigned char *uuid, char *dest)
	{
		size_t len = 0;

		for (size_t i = 0; i < 16; i++)
		{
			if (i == 4 || i == 6 || i == 8 || i == 10)
				dest[len++] = '-';
		
			hexToChar((uuid[i] & 0xf0) >> 4, &dest[len++]);
			hexToChar(uuid[i] & 0x0f, &dest[len++]);
		}

		dest[len] = 0;
	}

	void stModeToStr(unsigned int mode, char *dest)
	{
		if (mode & S_IFDIR)
			dest[0] = 'd';
		else if (mode & S_IFBLK)
			dest[0] = 'b';
		else if (mode & S_IFCHR)
			dest[0] = 'c';
		/* Btrfs seems to mark all files as symlinks for some reason */
		/*else if (mode & S_IFLNK)
			dest[0] = 'l';*/
		else if (mode & S_IFIFO)
			dest[0] = 'p';
		/* Btrfs also appears to enjoy marking everything as a socket */
		/*else if (mode & S_IFSOCK)
			dest[0] = 's';*/
		else
			dest[0] = '-';

		dest[1] = (mode & S_IRUSR) ? 'r' : '-';
		dest[2] = (mode & S_IWUSR) ? 'w' : '-';

		if (mode & S_ISUID)
			dest[3] = (mode & S_IXUSR) ? 's' : 'S';
		else
			dest[3] = (mode & S_IXUSR) ? 'x' : '-';

		dest[4] = (mode & S_IRGRP) ? 'r' : '-';
		dest[5] = (mode & S_IWGRP) ? 'w' : '-';

		if (mode & S_ISGID)
			dest[6] = (mode & S_IXUSR) ? 's' : 'S';
		else
			dest[6] = (mode & S_IXUSR) ? 'x' : '-';

		dest[7] = (mode & S_IROTH) ? 'r' : '-';
		dest[8] = (mode & S_IWOTH) ? 'w' : '-';

		if (mode & S_ISVTX)
			dest[9] = (mode & S_IXUSR) ? 't' : 'T';
		else
			dest[9] = (mode & S_IXUSR) ? 'x' : '-';

		dest[10] = 0;
	}

	void bgFlagsToStr(BlockGroupFlags flags, char *dest)
	{
		/* must be DATA, SYSTEM, or METADATA; nothing else! */
		assert((flags & BGFLAG_DATA) || (flags & BGFLAG_SYSTEM) || (flags & BGFLAG_METADATA));

		if (flags & BGFLAG_DATA)
			strcpy(dest, "DATA");
		else if (flags & BGFLAG_SYSTEM)
			strcpy(dest, "SYSTEM");
		else if (flags & BGFLAG_METADATA)
			strcpy(dest, "METADATA");

		strcat(dest, " ");

		if (flags & BGFLAG_RAID0)
			strcat(dest, "RAID0");
		else if (flags & BGFLAG_RAID1)
			strcat(dest, "RAID1");
		else if (flags & BGFLAG_DUPLICATE)
			strcat(dest, "duplicate");
		else if (flags & BGFLAG_RAID10)
			strcat(dest, "RAID10");
		else
			strcat(dest, "single");
	}

	void dokanError(int dokanResult)
	{
		switch (dokanResult)
		{
		case DOKAN_SUCCESS:
			printf("Dokan terminated successfully.\n");
			break;
		case DOKAN_ERROR:
			printf("Dokan reported a general error!\n");
			break;
		case DOKAN_DRIVE_LETTER_ERROR:
			printf("Dokan reported a bad drive letter!\n");
			break;
		case DOKAN_DRIVER_INSTALL_ERROR:
			printf("Dokan reported it couldn't install the driver!\n");
			break;
		case DOKAN_START_ERROR:
			printf("Dokan reported something is wrong with the driver!\n");
			break;
		case DOKAN_MOUNT_ERROR:
			printf("Dokan reported it couldn't assign a drive letter or mount point!\n"
				"Sometimes this problem is resolved by running WinBtrfsCLI again.\n");
			break;
		case DOKAN_MOUNT_POINT_ERROR:
			printf("Dokan reported the mount point is invalid!\n");
			break;
		default:
			printf("Dokan returned an unknown error!\n");
			break;
		}
	}
}
