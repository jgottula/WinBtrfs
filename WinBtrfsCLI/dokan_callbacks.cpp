/* dokan_callbacks.cpp
 * implementations of Dokan callback functions
 *
 * WinBtrfs
 *
 * Copyright (c) 2011 Justin Gottula
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <Windows.h>
#include <dokan.h>
#include "constants.h"
#include "structures.h"
#include "endian.h"
#include "btrfs_system.h"
#include "btrfs_operations.h"

extern BtrfsSuperblock super;

HANDLE hBigDokanLock = INVALID_HANDLE_VALUE;

DWORD setupBigDokanLock()
{
	hBigDokanLock = CreateMutex(NULL, FALSE, NULL);

	if (hBigDokanLock == INVALID_HANDLE_VALUE)
		return GetLastError();
	else
		return ERROR_SUCCESS;
}

// CreateFile
//   If file is a directory, CreateFile (not OpenDirectory) may be called.
//   In this case, CreateFile should return 0 when that directory can be opened.
//   You should set TRUE on DokanFileInfo->IsDirectory when file is a directory.
//   When CreationDisposition is CREATE_ALWAYS or OPEN_ALWAYS and a file already exists,
//   you should return ERROR_ALREADY_EXISTS(183) (not negative value)
int DOKAN_CALLBACK btrfsCreateFile(LPCWSTR fileName, DWORD desiredAccess, DWORD shareMode, DWORD creationDisposition,
	DWORD flagsAndAttributes, PDOKAN_FILE_INFO info)
{
	wprintf(L"btrfsCreateFile: unimplemented! [%s]\n", fileName);
	
	return ERROR_SUCCESS;
}

int DOKAN_CALLBACK btrfsOpenDirectory(LPCWSTR fileName, PDOKAN_FILE_INFO info)
{
	printf("btrfsOpenDirectory: unimplemented! [%s]\n", fileName);
	
	return ERROR_SUCCESS;
}

int DOKAN_CALLBACK btrfsCreateDirectory(LPCWSTR fileName, PDOKAN_FILE_INFO info)
{
	printf("btrfsCreateDirectory: SHOULD NEVER BE CALLED!! [%s]\n", fileName);
	
	return ERROR_SUCCESS;
}

// When FileInfo->DeleteOnClose is true, you must delete the file in Cleanup.
int DOKAN_CALLBACK btrfsCleanup(LPCWSTR fileName, PDOKAN_FILE_INFO info)
{
	printf("btrfsCleanup: unimplemented! [%s]\n", fileName);
	
	return ERROR_SUCCESS;
}

int DOKAN_CALLBACK btrfsCloseFile(LPCWSTR fileName, PDOKAN_FILE_INFO info)
{
	printf("btrfsCloseFile: unimplemented! [%s]\n", fileName);
	
	return ERROR_SUCCESS;
}

int DOKAN_CALLBACK btrfsReadFile(LPCWSTR fileName, LPVOID buffer, DWORD numberOfBytesToRead, LPDWORD numberOfBytesRead,
	LONGLONG offset, PDOKAN_FILE_INFO info)
{
	printf("btrfsReadFile: unimplemented! [%s]\n", fileName);

	return ERROR_SUCCESS;
}

int DOKAN_CALLBACK btrfsWriteFile(LPCWSTR fileName, LPCVOID buffer, DWORD numberOfBytesToWrite, LPDWORD numberOfBytesWritten,
	LONGLONG offset, PDOKAN_FILE_INFO info)
{
	printf("btrfsWriteFile: SHOULD NEVER BE CALLED!! [%s]\n", fileName);

	return ERROR_SUCCESS;
}

int DOKAN_CALLBACK btrfsFlushFileBuffers(LPCWSTR fileName, PDOKAN_FILE_INFO info)
{
	printf("btrfsFlushFileBuffers: SHOULD NEVER BE CALLED!! [%s]\n", fileName);

	return ERROR_SUCCESS;
}

int DOKAN_CALLBACK btrfsGetFileInformation(LPCWSTR fileName, LPBY_HANDLE_FILE_INFORMATION buffer, PDOKAN_FILE_INFO info)
{
	char fileNameB[MAX_PATH];
	BtrfsObjID objectID;
	Inode inode;

	if (WaitForSingleObject(hBigDokanLock, 10000) != WAIT_OBJECT_0)
		return -ERROR_SEM_TIMEOUT; // error code looks sketchy

	assert(wcstombs(fileNameB, fileName, MAX_PATH) == wcslen(fileName));

	if (getPathID(fileNameB, &objectID) != 0)
	{
		ReleaseMutex(hBigDokanLock);
		return -ERROR_FILE_NOT_FOUND;
	}
	if (getInode(objectID, &inode, 1) != 0)
	{
		ReleaseMutex(hBigDokanLock);
		return -ERROR_FILE_NOT_FOUND;
	}
	
	/* TODO: FILE_ATTRIBUTE_COMPRESSED, FILE_ATTRIBUTE_SPARSE_FILE, FILE_ATTRIBUTE_REPARSE_POINT (maybe) */
	buffer->dwFileAttributes = 0;
	if (endian32(inode.inodeItem.stMode) & S_IFBLK) buffer->dwFileAttributes |= FILE_ATTRIBUTE_DEVICE; // is this right?
	if (endian32(inode.inodeItem.stMode) & S_IFDIR) buffer->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
	if (inode.hidden) buffer->dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN;

	/* not sure if this is necessary, but it seems to be what you're supposed to do */
	if (buffer->dwFileAttributes == 0)
		buffer->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;

	convertTime(&inode.inodeItem.stCTime, &buffer->ftCreationTime);
	convertTime(&inode.inodeItem.stATime, &buffer->ftLastAccessTime);
	convertTime(&inode.inodeItem.stMTime, &buffer->ftLastWriteTime);
	
	/* using the last 4 bytes of the UUID */
	buffer->dwVolumeSerialNumber = super.uuid[0] + (super.uuid[1] << 8) + (super.uuid[2] << 16) + (super.uuid[3] << 24);

	buffer->nFileSizeHigh = (DWORD)(endian64(inode.inodeItem.stSize) >> 32);
	buffer->nFileSizeLow = (DWORD)endian64(inode.inodeItem.stSize);

	buffer->nNumberOfLinks = endian32(inode.inodeItem.stNLink);

	buffer->nFileIndexHigh = (DWORD)(endian64(inode.objectID) << 32);
	buffer->nFileIndexLow = (DWORD)endian64(inode.objectID);
	
	ReleaseMutex(hBigDokanLock);

	return ERROR_SUCCESS;
}

int DOKAN_CALLBACK btrfsFindFiles(LPCWSTR pathName, PFillFindData pFillFindData, PDOKAN_FILE_INFO info)
{
	char pathNameB[MAX_PATH];
	wchar_t nameW[MAX_PATH];
	BtrfsObjID objectID;
	DirList listing;
	WIN32_FIND_DATAW findData;
	int i;

	if (WaitForSingleObject(hBigDokanLock, 10000) != WAIT_OBJECT_0)
		return -ERROR_SEM_TIMEOUT; // error code looks sketchy

	assert(wcstombs(pathNameB, pathName, MAX_PATH) == wcslen(pathName));
	
	if (getPathID(pathNameB, &objectID) != 0)
	{
		ReleaseMutex(hBigDokanLock);
		return -ERROR_PATH_NOT_FOUND;
	}

	if (dirList(objectID, &listing) != 0)
	{
		ReleaseMutex(hBigDokanLock);
		return -ERROR_PATH_NOT_FOUND; // probably not an adequate error code
	}

	for (i = 0; i < listing.numEntries; i++)
	{
		/* TODO: FILE_ATTRIBUTE_COMPRESSED, FILE_ATTRIBUTE_SPARSE_FILE, FILE_ATTRIBUTE_REPARSE_POINT (maybe) */
		findData.dwFileAttributes = 0;
		if (endian32(listing.inodes[i].inodeItem.stMode) & S_IFBLK) findData.dwFileAttributes |= FILE_ATTRIBUTE_DEVICE; // is this right?
		if (endian32(listing.inodes[i].inodeItem.stMode) & S_IFDIR) findData.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
		if (listing.inodes[i].hidden) findData.dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN;

		/* not sure if this is necessary, but it seems to be what you're supposed to do */
		if (findData.dwFileAttributes == 0)
			findData.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
		
		convertTime(&(listing.inodes[i].inodeItem.stCTime), &findData.ftCreationTime);
		convertTime(&(listing.inodes[i].inodeItem.stATime), &findData.ftLastAccessTime);
		convertTime(&(listing.inodes[i].inodeItem.stMTime), &findData.ftLastWriteTime);

		findData.nFileSizeHigh = (DWORD)(endian64(listing.inodes[i].inodeItem.stSize) >> 32);
		findData.nFileSizeLow = (DWORD)endian64(listing.inodes[i].inodeItem.stSize);

		assert(mbstowcs(nameW, listing.names[i], MAX_PATH) == strlen(listing.names[i]));

		wcscpy(findData.cFileName, nameW);
		findData.cAlternateFileName[0] = 0; // no 8.3 name

		/* assuming that calling pFillFindData multiple times with the same pointer arg will work as intended */

		/* call the function pointer */
		(*pFillFindData)(&findData, info);
	}

	destroyDirList(&listing);

	ReleaseMutex(hBigDokanLock);

	return ERROR_SUCCESS;
}

int DOKAN_CALLBACK btrfsSetFileAttributes(LPCWSTR fileName, DWORD fileAttributes, PDOKAN_FILE_INFO info)
{
	printf("btrfsSetFileAttributes: SHOULD NEVER BE CALLED!! [%s]\n", fileName);

	return ERROR_SUCCESS;
}

int DOKAN_CALLBACK btrfsSetFileTime(LPCWSTR fileName, CONST FILETIME *creationTime, CONST FILETIME *lastAccessTime,
	CONST FILETIME *lastWriteTime, PDOKAN_FILE_INFO info)
{
	printf("btrfsSetFileTime: SHOULD NEVER BE CALLED!! [%s]\n", fileName);

	return ERROR_SUCCESS;
}

// You should not delete file on DeleteFile or DeleteDirectory.
// When DeleteFile or DeleteDirectory, you must check whether
// you can delete the file or not, and return 0 (when you can delete it)
// or appropriate error codes such as -ERROR_DIR_NOT_EMPTY,
// -ERROR_SHARING_VIOLATION.
// When you return 0 (ERROR_SUCCESS), you get Cleanup with
// FileInfo->DeleteOnClose set TRUE and you have to delete the
// file in Close.
int DOKAN_CALLBACK btrfsDeleteFile(LPCWSTR fileName, PDOKAN_FILE_INFO info)
{
	printf("btrfsDeleteFile: SHOULD NEVER BE CALLED!! [%s]\n", fileName);

	return ERROR_SUCCESS;
}

// You should not delete file on DeleteFile or DeleteDirectory.
// When DeleteFile or DeleteDirectory, you must check whether
// you can delete the file or not, and return 0 (when you can delete it)
// or appropriate error codes such as -ERROR_DIR_NOT_EMPTY,
// -ERROR_SHARING_VIOLATION.
// When you return 0 (ERROR_SUCCESS), you get Cleanup with
// FileInfo->DeleteOnClose set TRUE and you have to delete the
// file in Close.
int DOKAN_CALLBACK btrfsDeleteDirectory(LPCWSTR fileName, PDOKAN_FILE_INFO info)
{
	printf("btrfsDeleteDirectory: SHOULD NEVER BE CALLED!! [%s]\n", fileName);

	return ERROR_SUCCESS;
}

int DOKAN_CALLBACK btrfsMoveFile(LPCWSTR existingFileName, LPCWSTR newFileName, BOOL replaceExisting, PDOKAN_FILE_INFO info)
{
	printf("btrfsMoveFile: SHOULD NEVER BE CALLED!! [%s -> %s]\n", existingFileName, newFileName);

	return ERROR_SUCCESS;
}

int DOKAN_CALLBACK btrfsSetEndOfFile(LPCWSTR fileName, LONGLONG length, PDOKAN_FILE_INFO info)
{
	printf("btrfsSetEndOfFile: SHOULD NEVER BE CALLED!! [%s]\n", fileName);

	return ERROR_SUCCESS;
}

int DOKAN_CALLBACK btrfsSetAllocationSize(LPCWSTR fileName, LONGLONG length, PDOKAN_FILE_INFO info)
{
	printf("btrfsSetAllocationSize: SHOULD NEVER BE CALLED!! [%s]\n", fileName);

	return ERROR_SUCCESS;
}

int DOKAN_CALLBACK btrfsLockFile(LPCWSTR fileName, LONGLONG byteOffset, LONGLONG length, PDOKAN_FILE_INFO info)
{
	printf("btrfsLockFile: unimplemented! [%s]\n", fileName);

	return ERROR_SUCCESS;
}

int DOKAN_CALLBACK btrfsUnlockFile(LPCWSTR fileName, LONGLONG byteOffset, LONGLONG length, PDOKAN_FILE_INFO info)
{
	printf("btrfsUnlockFile: unimplemented! [%s]\n", fileName);

	return ERROR_SUCCESS;
}

int DOKAN_CALLBACK btrfsGetDiskFreeSpace(PULONGLONG freeBytesAvailable, PULONGLONG totalNumberOfBytes,
	PULONGLONG totalNumberOfFreeBytes, PDOKAN_FILE_INFO info)
{
	ULONGLONG free, total;

	/* Big Dokan Lock not needed here */

	total = endian64(super.totalBytes);
	free =  total - endian64(super.bytesUsed);
	
	*freeBytesAvailable = free;
	*totalNumberOfBytes = total;
	*totalNumberOfFreeBytes = free;

	return ERROR_SUCCESS;
}

int DOKAN_CALLBACK btrfsGetVolumeInformation(LPWSTR volumeNameBuffer, DWORD volumeNameSize, LPDWORD volumeSerialNumber,
	LPDWORD maximumComponentLength, LPDWORD fileSystemFlags, LPWSTR fileSystemNameBuffer, DWORD fileSystemNameSize,
	PDOKAN_FILE_INFO info)
{
	CHAR labelS[MAX_PATH + 1];
	
	/* Big Dokan Lock not needed here */

	/* TODO: switch to strcpy_s & mbstowcs_s; this currently causes pointers to go bad,
		which presumably indicates some sort of vulnerability in the present code that
		the *_s functions are systematically preventing by padding with 0xfefefefe etc. */
	strcpy(labelS, super.label);
	mbstowcs(volumeNameBuffer, labelS, volumeNameSize);

	/* using the last 4 bytes of the UUID */
	*volumeSerialNumber = super.uuid[0] + (super.uuid[1] << 8) + (super.uuid[2] << 16) + (super.uuid[3] << 24);

	*maximumComponentLength = 255;

	/* change these flags as features are added: e.g. extended metadata, compression, rw support, ... */
	*fileSystemFlags = FILE_CASE_PRESERVED_NAMES | FILE_CASE_SENSITIVE_SEARCH | FILE_READ_ONLY_VOLUME;

	/* TODO: switch to wcscpy_s */
	wcscpy(fileSystemNameBuffer, L"Btrfs");

	return ERROR_SUCCESS;
}

int DOKAN_CALLBACK btrfsUnmount(PDOKAN_FILE_INFO info)
{
	/* nothing to do */
	
	return ERROR_SUCCESS;
}

int DOKAN_CALLBACK btrfsGetFileSecurity(LPCWSTR fileName, PSECURITY_INFORMATION secInfo, PSECURITY_DESCRIPTOR secDesc,
	ULONG secDescLen, PULONG lengthNeeded, PDOKAN_FILE_INFO info)
{
	printf("btrfsGetFileSecurity: unimplemented! [%s]\n", fileName);

	return ERROR_SUCCESS;
}

int DOKAN_CALLBACK btrfsSetFileSecurity(LPCWSTR fileName, PSECURITY_INFORMATION secInfo, PSECURITY_DESCRIPTOR secDesc,
	ULONG secDescLen, PDOKAN_FILE_INFO info)
{
	printf("btrfsSetFileSecurity: SHOULD NEVER BE CALLED!! [%s]\n", fileName);

	return ERROR_SUCCESS;
}
