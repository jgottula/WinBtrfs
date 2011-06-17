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
#include <list>
#include <Windows.h>
#include <dokan.h>
#include "constants.h"
#include "structures.h"
#include "endian.h"
#include "util.h"
#include "btrfs_system.h"
#include "btrfs_operations.h"

extern BtrfsSuperblock super;

std::list<FilePkg> openFiles;
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
	char fileNameB[MAX_PATH];
	BtrfsObjID objectID;
	FilePkg filePkg;

	assert(creationDisposition != CREATE_ALWAYS && creationDisposition != OPEN_ALWAYS);
	assert(wcstombs(fileNameB, fileName, MAX_PATH) == wcslen(fileName));

	/* just in case */
	info->Context = 0x0;
	
	if (WaitForSingleObject(hBigDokanLock, 10000) != WAIT_OBJECT_0)
	{
		printf("btrfsCreateFile: couldn't get ownership of the Big Dokan Lock! [%S]\n", fileName);
		return -ERROR_SEM_TIMEOUT; // error code looks sketchy
	}

	if (getPathID(fileNameB, &objectID) != 0)
	{
		ReleaseMutex(hBigDokanLock);
		printf("btrfsCreateFile: getPathID failed! [%S]\n", fileName);
		return -ERROR_FILE_NOT_FOUND;
	}

	if (parseFSTree(FSOP_GET_FILE_PKG, &objectID, NULL, NULL, &filePkg, NULL) != 0)
	{
		ReleaseMutex(hBigDokanLock);
		printf("btrfsCreateFile: parseFSTree with FSOP_GET_FILE_PKG failed! [%S]\n", fileName);
		return -ERROR_FILE_NOT_FOUND;
	}
	
	ReleaseMutex(hBigDokanLock);

	std::list<FilePkg>::iterator it = openFiles.begin(), end = openFiles.end();
	for ( ; it != end; ++it)
	{
		/* there should not be any duplicate records in this array! */
		//assert(it->objectID != objectID);
		/* actually, that's a *terrible* assumption. for now, we'll allow dupes under the knowledge that they will
			contain exactly the same information, so it doesn't matter which one gets cleared up later. this should
			be totally cleared up when we move over to a multimap container instead of the current linked list. */
		
		if (it->objectID >= objectID)
			break;
	}

	openFiles.insert(it, filePkg);

	info->Context = objectID;

	if (!info->IsDirectory && (filePkg.inode.stMode & S_IFDIR))
		info->IsDirectory = TRUE;

	/* TODO: respect the desiredAccess parameter and lock the file based on shareMode */
	
	printf("btrfsCreateFile: OK [%S]\n", fileName);
	return ERROR_SUCCESS;
}

/* TODO: merge this with btrfsCreateFile */
int DOKAN_CALLBACK btrfsOpenDirectory(LPCWSTR fileName, PDOKAN_FILE_INFO info)
{
	char fileNameB[MAX_PATH];
	BtrfsObjID objectID;
	FilePkg filePkg;
	
	assert(wcstombs(fileNameB, fileName, MAX_PATH) == wcslen(fileName));

	/* just in case */
	info->Context = 0x0;
	
	if (WaitForSingleObject(hBigDokanLock, 10000) != WAIT_OBJECT_0)
	{
		printf("btrfsOpenDirectory: couldn't get ownership of the Big Dokan Lock! [%S]\n", fileName);
		return -ERROR_SEM_TIMEOUT; // error code looks sketchy
	}

	if (getPathID(fileNameB, &objectID) != 0)
	{
		ReleaseMutex(hBigDokanLock);
		printf("btrfsOpenDirectory: getPathID failed! [%S]\n", fileName);
		return -ERROR_FILE_NOT_FOUND;
	}

	if (parseFSTree(FSOP_GET_FILE_PKG, &objectID, NULL, NULL, &filePkg, NULL) != 0)
	{
		ReleaseMutex(hBigDokanLock);
		printf("btrfsOpenDirectory: parseFSTree with FSOP_GET_FILE_PKG failed! [%S]\n", fileName);
		return -ERROR_FILE_NOT_FOUND;
	}
	
	ReleaseMutex(hBigDokanLock);

	std::list<FilePkg>::iterator it = openFiles.begin(), end = openFiles.end();
	for ( ; it != end; ++it)
	{
		/* there should not be any duplicate records in this array! */
		//assert(it->objectID != objectID);
		/* actually, that's a *terrible* assumption. for now, we'll allow dupes under the knowledge that they will
			contain exactly the same information, so it doesn't matter which one gets cleared up later. this should
			be totally cleared up when we move over to a multimap container instead of the current linked list. */
		
		if (it->objectID >= objectID)
			break;
	}

	openFiles.insert(it, filePkg);

	info->Context = objectID;

	if (!info->IsDirectory && (filePkg.inode.stMode & S_IFDIR))
		info->IsDirectory = TRUE;
	
	printf("btrfsOpenDirectory: OK [%S]\n", fileName);
	return ERROR_SUCCESS;
}

int DOKAN_CALLBACK btrfsCreateDirectory(LPCWSTR fileName, PDOKAN_FILE_INFO info)
{
	printf("btrfsCreateDirectory: SHOULD NEVER BE CALLED!! [%s]\n", fileName);
	
	return ERROR_SUCCESS;
}

int DOKAN_CALLBACK btrfsCleanup(LPCWSTR fileName, PDOKAN_FILE_INFO info)
{
	std::list<FilePkg>::iterator it = openFiles.begin(), end = openFiles.end();
	for ( ; it != end; ++it)
	{
		if (it->objectID == (BtrfsObjID)info->Context)
			break;
	}

	/* we should always be able to find an entry in openFiles */
	assert(it != end);

	free(it->extentData);
	openFiles.erase(it);
	
	printf("btrfsCleanup: OK [%s]\n", fileName);
	return ERROR_SUCCESS;
}

int DOKAN_CALLBACK btrfsCloseFile(LPCWSTR fileName, PDOKAN_FILE_INFO info)
{
	printf("btrfsCloseFile: OK [%s]\n", fileName);
	return ERROR_SUCCESS;
}

// this may be called AFTER Cleanup in some cases in order to complete IO operations
int DOKAN_CALLBACK btrfsReadFile(LPCWSTR fileName, LPVOID buffer, DWORD numberOfBytesToRead, LPDWORD numberOfBytesRead,
	LONGLONG offset, PDOKAN_FILE_INFO info)
{
	printf("btrfsReadFile: unimplemented! [%s]\n", fileName);

	/* TODO: probably DON'T go to all the trouble of parsing the extent tree IN THIS FUNCTION; get that information
		ahead of time (or maybe just on the FIRST read so we don't do it needlessly on files that aren't read)
		and store it in openFiles or somewhere else */

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
	/* Big Dokan Lock not needed here */

	std::list<FilePkg>::iterator it = openFiles.begin(), end = openFiles.end();
	for ( ; it != end; ++it)
	{
		if (it->objectID == (BtrfsObjID)info->Context)
			break;
	}

	/* failing to find the element is NOT an option */
	assert(it != end);

	/* TODO: FILE_ATTRIBUTE_COMPRESSED, FILE_ATTRIBUTE_SPARSE_FILE, FILE_ATTRIBUTE_REPARSE_POINT (maybe) */
	buffer->dwFileAttributes = 0;
	if (endian32(it->inode.stMode) & S_IFBLK) buffer->dwFileAttributes |= FILE_ATTRIBUTE_DEVICE; // is this right?
	if (endian32(it->inode.stMode) & S_IFDIR) buffer->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
	if (it->hidden) buffer->dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN;

	/* not sure if this is necessary, but it seems to be what you're supposed to do */
	if (buffer->dwFileAttributes == 0)
		buffer->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;

	convertTime(&it->inode.stCTime, &buffer->ftCreationTime);
	convertTime(&it->inode.stATime, &buffer->ftLastAccessTime);
	convertTime(&it->inode.stMTime, &buffer->ftLastWriteTime);
	
	/* using the least significant 4 bytes of the UUID */
	buffer->dwVolumeSerialNumber = super.uuid[0] + (super.uuid[1] << 8) + (super.uuid[2] << 16) + (super.uuid[3] << 24);

	buffer->nFileSizeHigh = (DWORD)(endian64(it->inode.stSize) >> 32);
	buffer->nFileSizeLow = (DWORD)endian64(it->inode.stSize);

	buffer->nNumberOfLinks = endian32(it->inode.stNLink);

	buffer->nFileIndexHigh = (DWORD)(endian64(it->objectID) << 32);
	buffer->nFileIndexLow = (DWORD)endian64(it->objectID);
	
	printf("btrfsGetFileInformation: OK [%S]\n", fileName);
	return ERROR_SUCCESS;
}

int DOKAN_CALLBACK btrfsFindFiles(LPCWSTR pathName, PFillFindData pFillFindData, PDOKAN_FILE_INFO info)
{
	char pathNameB[MAX_PATH];
	wchar_t nameW[MAX_PATH];
	BtrfsObjID objectID = (BtrfsObjID)info->Context;
	DirList dirList;
	WIN32_FIND_DATAW findData;

	assert(wcstombs(pathNameB, pathName, MAX_PATH) == wcslen(pathName));

	std::list<FilePkg>::iterator it = openFiles.begin(), end = openFiles.end();
	for ( ; it != end; ++it)
	{
		/* return ERROR_DIRECTORY (267) if attempting to dirlist a file; this is what NTFS does */
		if (it->objectID == objectID && !(it->inode.stMode & S_IFDIR))
		{
			printf("btrfsFindFiles: expected a dir but was given a file! [%S]\n", pathName);
			return -ERROR_DIRECTORY; // for some reason, ERROR_FILE_NOT_FOUND is reported to FindFirstFile
		}
	}

	if (WaitForSingleObject(hBigDokanLock, 10000) != WAIT_OBJECT_0)
	{
		printf("btrfsFindFiles: couldn't get ownership of the Big Dokan Lock! [%S]\n", pathName);
		return -ERROR_SEM_TIMEOUT; // error code looks sketchy
	}

	if (parseFSTree(FSOP_DIR_LIST, &objectID, NULL, NULL, &dirList, NULL) != 0)
	{
		ReleaseMutex(hBigDokanLock);
		printf("btrfsFindFiles: parseFSTree with FSOP_DIR_LIST failed! [%S]\n", pathName);
		return -ERROR_PATH_NOT_FOUND; // probably not an adequate error code
	}
	
	ReleaseMutex(hBigDokanLock);

	for (int i = 0; i < dirList.numEntries; i++)
	{
		/* TODO: extract this stuff out into a separate conversion function */
		
		/* TODO: FILE_ATTRIBUTE_COMPRESSED, FILE_ATTRIBUTE_SPARSE_FILE, FILE_ATTRIBUTE_REPARSE_POINT (maybe) */
		findData.dwFileAttributes = 0;
		if (endian32(dirList.entries[i].inode.stMode) & S_IFBLK) findData.dwFileAttributes |= FILE_ATTRIBUTE_DEVICE; // is this right?
		if (endian32(dirList.entries[i].inode.stMode) & S_IFDIR) findData.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
		if (dirList.entries[i].hidden) findData.dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN;

		/* not sure if this is necessary, but it seems to be what you're supposed to do */
		if (findData.dwFileAttributes == 0)
			findData.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
		
		convertTime(&(dirList.entries[i].inode.stCTime), &findData.ftCreationTime);
		convertTime(&(dirList.entries[i].inode.stATime), &findData.ftLastAccessTime);
		convertTime(&(dirList.entries[i].inode.stMTime), &findData.ftLastWriteTime);

		findData.nFileSizeHigh = (DWORD)(endian64(dirList.entries[i].inode.stSize) >> 32);
		findData.nFileSizeLow = (DWORD)endian64(dirList.entries[i].inode.stSize);

		assert(mbstowcs(nameW, dirList.entries[i].name, MAX_PATH) == strlen(dirList.entries[i].name));

		wcscpy(findData.cFileName, nameW);
		findData.cAlternateFileName[0] = 0; // no 8.3 name

		/* calling pFillFindData multiple times with the same pointer arg (but different contents) should work properly */

		/* call the function pointer */
		(*pFillFindData)(&findData, info);
	}
	
	free(dirList.entries);
	
	printf("btrfsFindFiles: OK [%S]\n", pathName);
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
	
	printf("btrfsGetDiskFreeSpace: OK\n");
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
	
	printf("btrfsGetVolumeInformation: OK\n");
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
