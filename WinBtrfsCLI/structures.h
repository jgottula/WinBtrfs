/* structures.h
 * struct declarations (on-disk and in-memory)
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

#pragma once

/* pack structs the way they are on the disk */
#pragma pack(1)

#include "constants.h"

/* ALL multibyte integers in Btrfs_____ structs WILL ALWAYS be little-endian!
	(use endian16(), endian32(), and endian64() to convert them)
	any other struct members WILL ALWAYS be in native endian! */

struct BtrfsTime
{
	__int64					secSince1970;
	unsigned int			nanoseconds;
};

struct BtrfsChecksum
{
	unsigned int			crc32c;
	unsigned char			padding				[0x1c];
};

struct BtrfsHeader
{
	BtrfsChecksum			csum;
	unsigned char			fsID				[0x10];
	unsigned __int64		blockNr;
	unsigned __int64		flags;

	unsigned char			chunkTreeUID		[0x10];
	unsigned __int64		generation;
	unsigned __int64		tree;
	unsigned int			nrItems;
	unsigned char			level;
};

struct BtrfsDiskKey
{
	BtrfsObjID				objectID;
	BtrfsItemType			type;
	unsigned __int64		offset;
};

struct BtrfsKeyPtr
{
	BtrfsDiskKey			key;
	unsigned __int64		blockNum;
	unsigned __int64		generation;
};

struct BtrfsItem
{
	BtrfsDiskKey			key;
	unsigned int			offset;
	unsigned int			size;
};

struct BtrfsInodeItem
{
	unsigned __int64		generation;
	unsigned __int64		transID;
	unsigned __int64		stSize;
	unsigned __int64		stBlocks;
	unsigned __int64		blockGroup;
	unsigned int			stNLink;
	unsigned int			stUID;
	unsigned int			stGID;
	unsigned int			stMode;
	unsigned __int64		stRDev;
	unsigned __int64		flags;
	unsigned __int64		sequence;
	unsigned char			reserved			[0x20];
	BtrfsTime				stATime;
	BtrfsTime				stCTime;
	BtrfsTime				stMTime;
	BtrfsTime				oTime;
};

struct BtrfsInodeRef
{
	unsigned __int64		index;
	unsigned short			nameLen;
	char					name				[0x0];
};

/* this structure may be repeated if multiple items have the same hash */
struct BtrfsDirItem
{
	BtrfsDiskKey			child;
	unsigned __int64		transID;
	unsigned short			m;
	unsigned short			n;
	unsigned char			childType;
	char					namePlusData		[0x0];

	/* namePlusData encompasses the following two variable-size items: */
	// char					name[n];
	// unsigned char		data[m];
};

/* same contents as BtrfsDirItem, but will never repeat */
typedef BtrfsDirItem BtrfsDirIndex;

struct BtrfsExtentData
{
	unsigned __int64		generation;
	unsigned __int64		n;
	unsigned char			compression;
	unsigned char			encryption;
	unsigned short			otherEncoding;
	unsigned char			type;
	unsigned char			inlineData			[0x0];
};

struct BtrfsExtentDataNonInline
{
	unsigned __int64		extAddr;
	unsigned __int64		extSize;
	unsigned __int64		offset;
	unsigned __int64		bytesInFile;
};

struct BtrfsRootItem
{
	BtrfsInodeItem			inodeItem;
	unsigned __int64		expectedGeneration;
	BtrfsObjID				objID;
	unsigned __int64		rootNodeBlockNum;
	unsigned __int64		byteLimit;
	unsigned __int64		bytesUsed;
	unsigned __int64		lastGenSnapshot;
	unsigned __int64		flags;
	unsigned int			numRefs;
	BtrfsDiskKey			dropProgress;
	unsigned char			dropLevel;
	unsigned char			rootLevel;
};

struct BtrfsRootRef
{
	BtrfsObjID				directoryID;
	unsigned __int64		sequence;
	unsigned short			n;
	char					name				[0x0];
};

typedef BtrfsRootRef BtrfsRootBackref;

/*struct BtrfsExtentItem
{
	unsigned __int64		refCount;
	unsigned __int64		generation;
	unsigned __int64		flags;
	BtrfsDiskKey			firstEntryKey;
	unsigned char			nodeLevel;
	// ...
};*/

struct BtrfsBlockGroupItem
{
	unsigned __int64		usedAmount;
	BtrfsObjID				chunkTreeID;
	unsigned __int64		flags;
};

struct BtrfsDevItem
{
	unsigned __int64		devID;
	unsigned __int64		numBytes;
	unsigned __int64		numBytesUsed;
	unsigned int			bestIOAlign;
	unsigned int			bestIOWidth;
	unsigned int			minIOSize;
	unsigned __int64		type;
	unsigned __int64		generation;
	unsigned __int64		startOffset;
	unsigned int			devGroup;
	unsigned char			seekSpeed;
	unsigned char			bandwidth;
	unsigned char			devUUID				[0x10];
	unsigned char			fsUUID				[0x10];
};

struct BtrfsChunkItemStripe
{
	unsigned __int64		devID;
	unsigned __int64		offset;
	unsigned char			devUUID				[0x10];
};

struct BtrfsChunkItem
{
	unsigned __int64		chunkSize;
	BtrfsObjID				rootObjIDref;
	unsigned __int64		stripeLen;
	unsigned __int64		type;
	unsigned int			bestIOAlign;
	unsigned int			bestIOWidth;
	unsigned int			minIOSize;
	unsigned short			numStripes;
	unsigned short			subStripes;
	BtrfsChunkItemStripe	stripes				[0x0];
};

struct BtrfsSBChunk
{
	BtrfsDiskKey			key;
	BtrfsChunkItem			chunkItem;
};

struct BtrfsSuperblock
{
	BtrfsChecksum			csum;
	unsigned char			uuid				[0x10];
	unsigned __int64		physAddr;
	unsigned __int64		flags;
	char					magic				[0x08];
	unsigned __int64		generation;
	unsigned __int64		rootTreeLAddr;
	unsigned __int64		chunkTreeLAddr;
	unsigned __int64		logTreeLAddr;
	unsigned __int64		logRootTransID;
	unsigned __int64		totalBytes;
	unsigned __int64		bytesUsed;
	BtrfsObjID				rootDirObjectID;
	unsigned __int64		numDevices;
	unsigned int			sectorSize;
	unsigned int			nodeSize;
	unsigned int			leafSize;
	unsigned int			stripeSize;
	unsigned int			n;
	unsigned __int64		chunkRootGeneration;
	unsigned __int64		compatFlags;
	unsigned __int64		compatROFlags;
	unsigned __int64		incompatFlags;
	unsigned short			csumType;
	unsigned char			rootLevel;
	unsigned char			chunkRootLevel;
	unsigned char			logRootLevel;
	BtrfsDevItem			devItem;
	char					label				[0x100];
	unsigned char			reserved			[0x100];
	unsigned char			chunkData			[0x800];
	unsigned char			unused				[0x4d5];
};

struct ItemPlus
{
	BtrfsItem				item;
	void					*data;
};

struct Root
{
	BtrfsObjID				objectID;
	BtrfsRootItem			rootItem;
};

struct FilePkg
{
	BtrfsObjID				objectID;
	BtrfsObjID				parentID; // not assigned by dir list
	BtrfsInodeItem			inode;
	BtrfsExtentData			*extentData; // not assigned by dir list
	char					name[256];
	bool					hidden;
};

struct DirList
{
	unsigned __int64		numEntries;
	FilePkg					*entries;
};
