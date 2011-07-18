/* WinBtrfsDrv/dokan_callbacks.h
 * implementations of Dokan callback functions
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
#include <dokan.h>

namespace WinBtrfsDrv
{
	DWORD setupBigDokanLock();
	int DOKAN_CALLBACK btrfsCreateFile(LPCWSTR fileName, DWORD desiredAccess, DWORD shareMode, DWORD creationDisposition,
		DWORD flagsAndAttributes, PDOKAN_FILE_INFO info);
	int DOKAN_CALLBACK btrfsOpenDirectory(LPCWSTR fileName, PDOKAN_FILE_INFO info);
	int DOKAN_CALLBACK btrfsCreateDirectory(LPCWSTR fileName, PDOKAN_FILE_INFO info);
	int DOKAN_CALLBACK btrfsCleanup(LPCWSTR fileName, PDOKAN_FILE_INFO info);
	int DOKAN_CALLBACK btrfsCloseFile(LPCWSTR fileName, PDOKAN_FILE_INFO info);
	int DOKAN_CALLBACK btrfsReadFile(LPCWSTR fileName, LPVOID buffer, DWORD numberOfBytesToRead, LPDWORD numberOfBytesRead,
		LONGLONG offset, PDOKAN_FILE_INFO info);
	int DOKAN_CALLBACK btrfsWriteFile(LPCWSTR fileName, LPCVOID buffer, DWORD numberOfBytesToWrite, LPDWORD numberOfBytesWritten,
		LONGLONG offset, PDOKAN_FILE_INFO info);
	int DOKAN_CALLBACK btrfsFlushFileBuffers(LPCWSTR fileName, PDOKAN_FILE_INFO info);
	int DOKAN_CALLBACK btrfsGetFileInformation(LPCWSTR fileName, LPBY_HANDLE_FILE_INFORMATION buffer, PDOKAN_FILE_INFO info);
	int DOKAN_CALLBACK btrfsFindFiles(LPCWSTR pathName, PFillFindData pFillFindData, PDOKAN_FILE_INFO info);
	int DOKAN_CALLBACK btrfsSetFileAttributes(LPCWSTR fileName, DWORD fileAttributes, PDOKAN_FILE_INFO info);
	int DOKAN_CALLBACK btrfsSetFileTime(LPCWSTR fileName, CONST FILETIME *creationTime, CONST FILETIME *lastAccessTime,
		CONST FILETIME *lastWriteTime, PDOKAN_FILE_INFO info);
	int DOKAN_CALLBACK btrfsDeleteFile(LPCWSTR fileName, PDOKAN_FILE_INFO info);
	int DOKAN_CALLBACK btrfsDeleteDirectory(LPCWSTR fileName, PDOKAN_FILE_INFO info);
	int DOKAN_CALLBACK btrfsMoveFile(LPCWSTR existingFileName, LPCWSTR newFileName, BOOL replaceExisting, PDOKAN_FILE_INFO info);
	int DOKAN_CALLBACK btrfsSetEndOfFile(LPCWSTR fileName, LONGLONG length, PDOKAN_FILE_INFO info);
	int DOKAN_CALLBACK btrfsSetAllocationSize(LPCWSTR fileName, LONGLONG length, PDOKAN_FILE_INFO info);
	int DOKAN_CALLBACK btrfsLockFile(LPCWSTR fileName, LONGLONG byteOffset, LONGLONG length, PDOKAN_FILE_INFO info);
	int DOKAN_CALLBACK btrfsUnlockFile(LPCWSTR fileName, LONGLONG byteOffset, LONGLONG length, PDOKAN_FILE_INFO info);
	int DOKAN_CALLBACK btrfsGetDiskFreeSpace(PULONGLONG freeBytesAvailable, PULONGLONG totalNumberOfBytes,
		PULONGLONG totalNumberOfFreeBytes, PDOKAN_FILE_INFO info);
	int DOKAN_CALLBACK btrfsGetVolumeInformation(LPWSTR volumeNameBuffer, DWORD volumeNameSize, LPDWORD volumeSerialNumber,
		LPDWORD maximumComponentLength, LPDWORD fileSystemFlags, LPWSTR fileSystemNameBuffer, DWORD fileSystemNameSize,
		PDOKAN_FILE_INFO info);
	int DOKAN_CALLBACK btrfsUnmount(PDOKAN_FILE_INFO info);
	int DOKAN_CALLBACK btrfsGetFileSecurity(LPCWSTR fileName, PSECURITY_INFORMATION secInfo, PSECURITY_DESCRIPTOR secDesc,
		ULONG secDescLen, PULONG lengthNeeded, PDOKAN_FILE_INFO info);
	int DOKAN_CALLBACK btrfsSetFileSecurity(LPCWSTR fileName, PSECURITY_INFORMATION secInfo, PSECURITY_DESCRIPTOR secDesc,
		ULONG secDescLen, PDOKAN_FILE_INFO info);
}
