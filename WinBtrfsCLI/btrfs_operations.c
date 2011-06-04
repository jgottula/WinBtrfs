/* btrfs_operations.c
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
#include "structures.h"
#include "endian.h"
#include "btrfs_filesystem.h"

extern BtrfsSuperblock super;

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
	
	return 0;
}

int DOKAN_CALLBACK btrfsOpenDirectory(LPCWSTR fileName, PDOKAN_FILE_INFO info)
{
	printf("btrfsOpenDirectory: unimplemented!\n");
	
	return 0;
}

int DOKAN_CALLBACK btrfsCreateDirectory(LPCWSTR fileName, PDOKAN_FILE_INFO info)
{
	printf("btrfsCreateDirectory: unimplemented!\n");
	
	return 0;
}

// When FileInfo->DeleteOnClose is true, you must delete the file in Cleanup.
int DOKAN_CALLBACK btrfsCleanup(LPCWSTR fileName, PDOKAN_FILE_INFO info)
{
	printf("btrfsCleanup: unimplemented!\n");
	
	return 0;
}

int DOKAN_CALLBACK btrfsCloseFile(LPCWSTR fileName, PDOKAN_FILE_INFO info)
{
	printf("btrfsCloseFile: unimplemented!\n");
	
	return 0;
}

int DOKAN_CALLBACK btrfsReadFile(LPCWSTR fileName, LPVOID buffer, DWORD numberOfBytesToRead, LPDWORD numberOfBytesRead,
	LONGLONG offset, PDOKAN_FILE_INFO info)
{
	printf("btrfsReadFile: unimplemented!\n");

	return 0;
}

int DOKAN_CALLBACK btrfsWriteFile(LPCWSTR fileName, LPCVOID buffer, DWORD numberOfBytesToWrite, LPDWORD numberOfBytesWritten,
	LONGLONG offset, PDOKAN_FILE_INFO info)
{
	printf("btrfsWriteFile: unimplemented!\n");

	return 0;
}

int DOKAN_CALLBACK btrfsFlushFileBuffers(LPCWSTR fileName, PDOKAN_FILE_INFO info)
{
	printf("btrfsFlushFileBuffers: unimplemented!\n");

	return 0;
}

int DOKAN_CALLBACK btrfsGetFileInformation(LPCWSTR fileName, LPBY_HANDLE_FILE_INFORMATION buffer, PDOKAN_FILE_INFO info)
{
	printf("btrfsGetFileInformation: unimplemented!\n");
	
	return 0;
}

// You should implement either FindFiles or FindFilesWithPattern
int DOKAN_CALLBACK btrfsFindFiles(LPCWSTR pathName, PFillFindData data, PDOKAN_FILE_INFO info)
{
	printf("btrfsFindFiles: unimplemented!\n");

	return 0;
}

// You should implement either FindFiles or FindFilesWithPattern
int DOKAN_CALLBACK btrfsFindFilesWithPattern(LPCWSTR pathName, LPCWSTR searchPattern, PFillFindData data, PDOKAN_FILE_INFO info)
{
	printf("btrfsFindFilesWithPattern: unimplemented!\n");

	return 0;
}

int DOKAN_CALLBACK btrfsSetFileAttributes(LPCWSTR fileName, DWORD fileAttributes, PDOKAN_FILE_INFO info)
{
	printf("btrfsSetFileAttributes: unimplemented!\n");

	return 0;
}

int DOKAN_CALLBACK btrfsSetFileTime(LPCWSTR fileName, CONST PFILETIME creationTime, CONST PFILETIME lastAccessTime,
	CONST PFILETIME lastWriteTime, PDOKAN_FILE_INFO info)
{
	printf("btrfsSetFileTime: unimplemented!\n");

	return 0;
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
	printf("btrfsDeleteFile: unimplemented!\n");

	return 0;
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
	printf("btrfsDeleteDirectory: unimplemented!\n");

	return 0;
}

int DOKAN_CALLBACK btrfsMoveFile(LPCWSTR existingFileName, LPCWSTR newFileName, BOOL replaceExisting, PDOKAN_FILE_INFO info)
{
	printf("btrfsMoveFile: unimplemented!\n");

	return 0;
}

int DOKAN_CALLBACK btrfsSetEndOfFile(LPCWSTR fileName, LONGLONG length, PDOKAN_FILE_INFO info)
{
	printf("btrfsSetEndOfFile: unimplemented!\n");

	return 0;
}

int DOKAN_CALLBACK btrfsSetAllocationSize(LPCWSTR fileName, LONGLONG length, PDOKAN_FILE_INFO info)
{
	printf("btrfsSetAllocationSize: unimplemented!\n");

	return 0;
}

int DOKAN_CALLBACK btrfsLockFile(LPCWSTR fileName, LONGLONG byteOffset, LONGLONG length, PDOKAN_FILE_INFO info)
{
	printf("btrfsLockFile: unimplemented!\n");

	return 0;
}

int DOKAN_CALLBACK btrfsUnlockFile(LPCWSTR fileName, LONGLONG byteOffset, LONGLONG length, PDOKAN_FILE_INFO info)
{
	printf("btrfsUnlockFile: unimplemented!\n");

	return 0;
}

int DOKAN_CALLBACK btrfsGetDiskFreeSpace(PULONGLONG freeBytesAvailable, PULONGLONG totalNumberOfBytes,
	PULONGLONG totalNumberOfFreeBytes, PDOKAN_FILE_INFO info)
{
	ULONGLONG free, total;

	total = endian64(super.totalBytes);
	free =  total - endian64(super.bytesUsed);

	printf("btrfsGetDiskFreeSpace: %I64u of %I64u KiB free\n", free / 1024, total / 1024);
	
	*freeBytesAvailable = free;
	*totalNumberOfBytes = total;
	*totalNumberOfFreeBytes = free;

	return 0;
}

int DOKAN_CALLBACK btrfsGetVolumeInformation(LPWSTR volumeNameBuffer, DWORD volumeNameSize, LPDWORD volumeSerialNumber,
	LPDWORD maximumComponentLength, LPDWORD fileSystemFlags, LPWSTR fileSystemNameBuffer, DWORD fileSystemNameSize,
	PDOKAN_FILE_INFO info)
{
	CHAR labelS[MAX_PATH + 1];

	printf("btrfsGetVolumeInformation\n");
	
	/* TODO: switch to strcpy_s & mbstowcs_s; this currently causes pointers to go bad,
		which presumably indicates some sort of vulnerability in the present code that
		the *_s functions are systematically preventing by padding with 0xfefefefe etc. */
	strcpy(labelS, super.label);
	mbstowcs(volumeNameBuffer, labelS, volumeNameSize);

	/* perhaps use the UUID here somehow in the future? */
	*volumeSerialNumber = 0;

	*maximumComponentLength = 255;

	/* change these flags as features are added: e.g. extended metadata, compression, rw support, ... */
	*fileSystemFlags = FILE_CASE_PRESERVED_NAMES | FILE_CASE_SENSITIVE_SEARCH | FILE_READ_ONLY_VOLUME;

	/* TODO: switch to wcscpy_s */
	wcscpy(fileSystemNameBuffer, L"Btrfs");

	return 0;
}

int DOKAN_CALLBACK btrfsUnmount(PDOKAN_FILE_INFO info)
{
	printf("btrfsUnmount\n");

	/* nothing to do for ro-mounted fs */
	
	return 0;
}

int DOKAN_CALLBACK btrfsGetFileSecurity(LPCWSTR fileName, PSECURITY_INFORMATION secInfo, PSECURITY_DESCRIPTOR secDesc,
	ULONG secDescLen, PULONG lengthNeeded, PDOKAN_FILE_INFO info)
{
	printf("btrfsGetFileSecurity: unimplemented!\n");

	return 0;
}

int DOKAN_CALLBACK btrfsSetFileSecurity(LPCWSTR fileName, PSECURITY_INFORMATION secInfo, PSECURITY_DESCRIPTOR secDesc,
	ULONG secDescLen, PDOKAN_FILE_INFO info)
{
	printf("btrfsSetFileSecurity: unimplemented!\n");

	return 0;
}
