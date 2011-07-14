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

#include "dokan_callbacks.h"
#include <cassert>
#include <list>
#include <vector>
#include "block_reader.h"
#include "btrfs_operations.h"
#include "compression.h"
#include "endian.h"
#include "fstree_parser.h"
#include "util.h"

extern std::vector<BlockReader *> blockReaders;
extern std::vector<BtrfsSuperblock> supers;
extern BtrfsObjID mountedSubvol;

std::list<FilePkg> openFiles, cleanedUpFiles;
HANDLE hBigDokanLock = INVALID_HANDLE_VALUE;

DWORD setupBigDokanLock()
{
	hBigDokanLock = CreateMutex(NULL, FALSE, NULL);

	if (hBigDokanLock == INVALID_HANDLE_VALUE)
		return GetLastError();
	else
		return ERROR_SUCCESS;
}

int btrfsCreateFileCommon(bool dir, LPCWSTR fileName, DWORD desiredAccess, DWORD shareMode, DWORD creationDisposition,
	DWORD flagsAndAttributes, PDOKAN_FILE_INFO info)
{
	char fileNameB[MAX_PATH];
	FileID parentID, *fileID = (FileID *)malloc(sizeof(FileID));
	FilePkg filePkg;

	if (!dir)
		assert(creationDisposition != CREATE_ALWAYS && creationDisposition != OPEN_ALWAYS);

	size_t result = wcstombs(fileNameB, fileName, MAX_PATH);
	assert(result == wcslen(fileName));

	/* just in case */
	info->Context = 0x0;
	
	if (WaitForSingleObject(hBigDokanLock, 10000) != WAIT_OBJECT_0)
	{
		printf("%s: couldn't get ownership of the Big Dokan Lock! [%S]\n",
			(dir ? "btrfsOpenDirectory" : "brtfsCreateFile"), fileName);
		return -ERROR_SEM_TIMEOUT; // error code looks sketchy
	}

	if (getPathID(fileNameB, fileID, &parentID) != 0)
	{
		ReleaseMutex(hBigDokanLock);
		printf("%s: getPathID failed! [%S]\n",
			(dir ? "btrfsOpenDirectory" : "brtfsCreateFile"), fileName);
		return -ERROR_FILE_NOT_FOUND;
	}

	int result2;
	if ((result2 = parseFSTree(fileID->treeID, FSOP_GET_FILE_PKG, &fileID->objectID, NULL, NULL, &filePkg, NULL)) != 0)
	{
		ReleaseMutex(hBigDokanLock);
		printf("%s: parseFSTree with FSOP_GET_FILE_PKG returned %d! [%S]\n",
			(dir ? "btrfsOpenDirectory" : "brtfsCreateFile"), result2, fileName);
		return -ERROR_FILE_NOT_FOUND;
	}
	
	ReleaseMutex(hBigDokanLock);

	/* populate the parent object ID */
	memcpy(&filePkg.parentID, &parentID, sizeof(FileID));

	/* populate the parent inode */
	int result3;
	if ((result3 = parseFSTree(filePkg.parentID.treeID, FSOP_GET_INODE, &filePkg.parentID.objectID,
		NULL, NULL, &filePkg.parentInode, NULL)) != 0)
	{
		ReleaseMutex(hBigDokanLock);
		printf("%s: parseFSTreewith FSOP_GET_INODE returned %d! [%S]\n",
			(dir ? "btrfsOpenDirectory" : "brtfsCreateFile"), result3, fileName);
		return -ERROR_FILE_NOT_FOUND;
	}

	std::list<FilePkg>::iterator it = openFiles.begin(), end = openFiles.end();
	for ( ; it != end; ++it)
	{
		/* there should not be any duplicate records in this array! */
		//assert(it->objectID != objectID);
		/* actually, that's a *terrible* assumption. for now, we'll allow dupes under the knowledge that they will
			contain exactly the same information, so it doesn't matter which one gets cleared up later. this should
			be totally cleared up when we move over to a multimap container instead of the current linked list. */
		
		/* sort the list in ascending treeID/objectID order */
		if (it->fileID.treeID > fileID->treeID ||
			(it->fileID.treeID == fileID->treeID && it->fileID.objectID > fileID->objectID))
			break;
	}

	openFiles.insert(it, filePkg);

	info->Context = (unsigned __int64)fileID;

	if (!info->IsDirectory && (filePkg.inode.stMode & S_IFDIR))
		info->IsDirectory = TRUE;

	if (!dir)
	{
		/* need to respect the desired access level, and lock the file based on shareMode */
		printf("btrfsCreateFile: TOOD: handle desiredAccess and shareMode\n");
	}
	
	printf("%s: OK [%S]\n", (dir ? "btrfsOpenDirectory" : "brtfsCreateFile"), fileName);
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
	return btrfsCreateFileCommon(false, fileName, desiredAccess, shareMode,
		creationDisposition, flagsAndAttributes, info);
}

int DOKAN_CALLBACK btrfsOpenDirectory(LPCWSTR fileName, PDOKAN_FILE_INFO info)
{
	return btrfsCreateFileCommon(true, fileName, 0, 0, 0, 0, info);
}

int DOKAN_CALLBACK btrfsCreateDirectory(LPCWSTR fileName, PDOKAN_FILE_INFO info)
{
	printf("btrfsCreateDirectory: SHOULD NEVER BE CALLED!! [%s]\n", fileName);
	
	return ERROR_SUCCESS;
}

int DOKAN_CALLBACK btrfsCleanup(LPCWSTR fileName, PDOKAN_FILE_INFO info)
{
	FileID *fileID = (FileID *)info->Context;
	
	std::list<FilePkg>::iterator it = openFiles.begin(), end = openFiles.end();
	for ( ; it != end; ++it)
	{
		if (it->fileID.treeID == fileID->treeID && it->fileID.objectID == fileID->objectID)
			break;
	}

	/* we should always be able to find an entry in openFiles */
	assert(it != end);

	FilePkg filePkg = *it;
	openFiles.erase(it);

	it = cleanedUpFiles.begin(), end = cleanedUpFiles.end();
	for ( ; it != end; ++it)
	{
		if (it->fileID.treeID == fileID->treeID && it->fileID.objectID == fileID->objectID)
			break;
	}
	
	/* insert the element such that the list is sorted in ascending object ID order */
	cleanedUpFiles.insert(it, filePkg);
	
	printf("btrfsCleanup: OK [%s]\n", fileName);
	return ERROR_SUCCESS;
}

int DOKAN_CALLBACK btrfsCloseFile(LPCWSTR fileName, PDOKAN_FILE_INFO info)
{
	FileID *fileID = (FileID *)info->Context;
	
	std::list<FilePkg>::iterator it = cleanedUpFiles.begin(), end = cleanedUpFiles.end();
	for ( ; it != end; ++it)
	{
		if (it->fileID.treeID == fileID->treeID && it->fileID.objectID == fileID->objectID)
			break;
	}

	/* we should always be able to find an entry in openFiles */
	assert(it != end);

	/* free this stuff on the heap */
	size_t numExtents = it->numExtents;
	for (size_t i = 0; i < numExtents; i++)
		free(it->extents[i].data);
	free(it->extents);

	cleanedUpFiles.erase(it);

	free(fileID);
	
	printf("btrfsCloseFile: OK [%s]\n", fileName);
	return ERROR_SUCCESS;
}

// this may be called AFTER Cleanup in some cases in order to complete IO operations
int DOKAN_CALLBACK btrfsReadFile(LPCWSTR fileName, LPVOID buffer, DWORD numberOfBytesToRead, LPDWORD numberOfBytesRead,
	LONGLONG offset, PDOKAN_FILE_INFO info)
{
	FileID *fileID = (FileID *)info->Context;
	FilePkg *filePkg;
	
	/* Big Dokan Lock not needed here */

	std::list<FilePkg>::iterator it = openFiles.begin(), end = openFiles.end();
	for ( ; it != end; ++it)
	{
		if (it->fileID.treeID == fileID->treeID && it->fileID.objectID == fileID->objectID)
			break;
	}

	if (it == end)
	{
		it = cleanedUpFiles.begin(), end = cleanedUpFiles.end();
		for ( ; it != end; ++it)
		{
			if (it->fileID.treeID == fileID->treeID && it->fileID.objectID == fileID->objectID)
				break;
		}

		/* failing to find the element is NOT an option */
		assert(it != end);
	}

	filePkg = &(*it);

	size_t numExtents = filePkg->numExtents;
	KeyedItem *extents = filePkg->extents;

	/* zero out the areas that don't get read in */
	memset(buffer, 0, numberOfBytesToRead);

	/* we'll read from this, so to be safe we'll set it first */
	*numberOfBytesRead = 0;

	unsigned __int64 readBegin = offset, readEnd = offset + numberOfBytesToRead;
	
	if (readBegin < filePkg->inode.stSize)
	{
		for (size_t i = 0; i < numExtents; i++)
		{
			BtrfsExtentData *extentData = (BtrfsExtentData *)extents[i].data;

			/* does the requested range include the first byte of this extent? */
			bool a = (endian64(extents[i].key.offset) >= offset &&
				endian64(extents[i].key.offset) < offset + numberOfBytesToRead);
			/* does the requested range include the last byte of this extent? */
			bool b = (endian64(extents[i].key.offset) + endian64(extentData->n) - 1 >= offset &&
				endian64(extents[i].key.offset) + endian64(extentData->n) - 1 < offset + numberOfBytesToRead);
			
			/* does the requested range start inside this extent? */
			bool first = !a && b;
			/* does the requested range end inside this extent? */
			bool last = a && !b;
			/* does the requested range take up the entirety of this extent? */
			bool span = a && b;
			/* does the requested range fit entirely within this extent? */
			bool within = (offset >= endian64(extents[i].key.offset) &&
				offset + numberOfBytesToRead <= endian64(extents[i].key.offset) + endian64(extentData->n));

			if (first || last || span || within)
			{
				assert(extentData->encryption == ENCRYPTION_NONE);
				assert(extentData->otherEncoding == ENCODING_NONE);
				
				if (extentData->compression <= COMPRESSION_LZO)
				{
					BtrfsExtentDataNonInline *nonInlinePart = NULL;
					unsigned char *compressed, *decompressed;
					size_t from, len;
					bool skipCopy = false;

					if (extentData->type == FILEDATA_INLINE)
						decompressed = extentData->inlineData;
					else
					{
						nonInlinePart = (BtrfsExtentDataNonInline *)extentData->inlineData;

						/* an address of zero indicates a sparse extent (i.e. all zeroes) */
						if (endian64(nonInlinePart->extAddr) == 0)
							skipCopy = true;
						else
						{
							printf("btrfsReadFile: warning: assuming first device!\n");
							
							compressed = (unsigned char *)malloc(endian64(nonInlinePart->extSize));
							DWORD result = blockReaders[0]->directRead(endian64(nonInlinePart->extAddr), ADDR_LOGICAL,
								endian64(nonInlinePart->extSize), compressed);
							assert(result == 0);

							switch (extentData->compression)
							{
							case COMPRESSION_NONE:
								/* transitive property: data is already decompressed! */
								decompressed = compressed;
								break;
							case COMPRESSION_ZLIB:
								decompressed = (unsigned char *)malloc(endian64(nonInlinePart->bytesInFile));
								
								if (zlibDecompress(compressed, decompressed, endian64(nonInlinePart->extSize),
									endian64(nonInlinePart->bytesInFile)) != 0)
								{
									printf("btrfsReadFile: zlib decompression failed!\n");
									free(compressed);
									free(decompressed);
									return PLA_E_CABAPI_FAILURE; // appopriate error code?
								}
								
								free(compressed);
								break;
							case COMPRESSION_LZO:
								decompressed = (unsigned char *)malloc(endian64(nonInlinePart->bytesInFile));
								
								if (lzoDecompress(compressed, decompressed, endian64(nonInlinePart->extSize),
									endian64(nonInlinePart->bytesInFile)) != 0)
								{
									printf("btrfsReadFile: lzo decompression failed!\n");
									free(compressed);
									free(decompressed);
									return PLA_E_CABAPI_FAILURE; // appopriate error code?
								}
								
								free(compressed);
								break;
							}
						}
					}

					if (span)
					{
						from = 0;
						len = extentData->n;
					}
					else if (within)
					{
						from = offset - endian64(extents[i].key.offset);
						len = numberOfBytesToRead;
					}
					else if (first)
					{
						from = offset - endian64(extents[i].key.offset);
						len = extentData->n - (offset - endian64(extents[i].key.offset));
					}
					else if (last)
					{
						from = 0;
						len = (offset + numberOfBytesToRead) - endian64(extents[i].key.offset);
					}

					if (!skipCopy)
					{
						memcpy((char *)buffer + *numberOfBytesRead, decompressed + from, len);

						if (extentData->type != FILEDATA_INLINE)
							free(decompressed);
					}
					
					numberOfBytesToRead -= len;
					*numberOfBytesRead += len;
					offset += len;

					/* that was the last extent (this assumes correct ordering of extents by offset) */
					if (last || within)
						break;
				}
				else
				{
					printf("btrfsReadFile: data is compressed with an unsupported algorithm!\n");
					return ERROR_UNSUPPORTED_COMPRESSION;
				}
			}
		}

		/* if the moronic application requested more data than the file contains,
			report a smaller read size to correct them */
		if (readEnd > filePkg->inode.stSize)
			*numberOfBytesRead -= readEnd - filePkg->inode.stSize;
	}
	else
		/* this idiotic fix courtesy of WordPad */
		*numberOfBytesRead = 0;

	printf("btrfsReadFile: OK [%s]\n", fileName);
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
	FileID *fileID = (FileID *)info->Context;
	
	/* Big Dokan Lock not needed here */

	std::list<FilePkg>::iterator it = openFiles.begin(), end = openFiles.end();
	for ( ; it != end; ++it)
	{
		if (it->fileID.treeID == fileID->treeID && it->fileID.objectID == fileID->objectID)
			break;
	}

	/* failing to find the element is NOT an option */
	assert(it != end);

	convertMetadata(&(*it), buffer, false);
	
	printf("btrfsGetFileInformation: OK [%S]\n", fileName);
	return ERROR_SUCCESS;
}

int DOKAN_CALLBACK btrfsFindFiles(LPCWSTR pathName, PFillFindData pFillFindData, PDOKAN_FILE_INFO info)
{
	char pathNameB[MAX_PATH];
	FileID *fileID = (FileID *)info->Context;
	FilePkg *filePkg;
	DirList dirList;
	WIN32_FIND_DATAW findData;
	bool root;

	size_t result = wcstombs(pathNameB, pathName, MAX_PATH);
	assert (result == wcslen(pathName));

	std::list<FilePkg>::iterator it = openFiles.begin(), end = openFiles.end();
	for ( ; it != end; ++it)
	{
		if (it->fileID.treeID == fileID->treeID && it->fileID.objectID == fileID->objectID)
		{
			/* return ERROR_DIRECTORY (267) if attempting to dirlist a file; this is what NTFS does */
			if (!(it->inode.stMode & S_IFDIR))
			{
				printf("btrfsFindFiles: expected a dir but was given a file! [%S]\n", pathName);
				return -ERROR_DIRECTORY; // for some reason, ERROR_FILE_NOT_FOUND is reported to FindFirstFile
			}

			break;
		}
	}

	/* failing to find the element is NOT an option */
	assert(it != end);

	filePkg = &(*it);

	if (WaitForSingleObject(hBigDokanLock, 10000) != WAIT_OBJECT_0)
	{
		printf("btrfsFindFiles: couldn't get ownership of the Big Dokan Lock! [%S]\n", pathName);
		return -ERROR_SEM_TIMEOUT; // error code looks sketchy
	}

	root = (strcmp(pathNameB, "\\") == 0);

	int result2;
	if ((result2 = parseFSTree(fileID->treeID, FSOP_DIR_LIST, filePkg, &root, NULL, &dirList, NULL)) != 0)
	{
		ReleaseMutex(hBigDokanLock);
		printf("btrfsFindFiles: parseFSTree with FSOP_DIR_LIST returned %d! [%S]\n", result2, pathName);
		return -ERROR_PATH_NOT_FOUND; // probably not an adequate error code
	}
	
	ReleaseMutex(hBigDokanLock);

	for (size_t i = 0; i < dirList.numEntries; i++)
	{
		convertMetadata(&dirList.entries[i], &findData, true);

		/* this function works OK if the same pointer (but different data) is passed in each time */
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

	total = endian64(supers[0].totalBytes);
	free =  total - endian64(supers[0].bytesUsed);
	
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

	/* switch to strcpy_s & mbstowcs_s; this currently causes pointers to go bad,
		which presumably indicates some sort of vulnerability in the present code that
		the *_s functions are systematically preventing by padding with 0xfefefefe etc. */
	printf("btrfsGetVolumeInformation: TODO: use secure string functions (1/2)\n");

	strcpy(labelS, supers[0].label);
	mbstowcs(volumeNameBuffer, labelS, volumeNameSize);

	/* using the last 4 bytes of the FS UUID */
	*volumeSerialNumber = supers[0].fsUUID[0] + (supers[0].fsUUID[1] << 8) +
		(supers[0].fsUUID[2] << 16) + (supers[0].fsUUID[3] << 24);

	*maximumComponentLength = 255;

	/* change these flags as features are added: e.g. extended metadata, compression, rw support, ... */
	*fileSystemFlags = FILE_CASE_PRESERVED_NAMES | FILE_CASE_SENSITIVE_SEARCH | FILE_READ_ONLY_VOLUME;
	
	printf("btrfsGetVolumeInformation: TODO: use secure string functions (2/2)\n");
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
