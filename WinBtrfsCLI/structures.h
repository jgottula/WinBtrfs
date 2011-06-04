/* structures.h
 * on-disk filesystem structures
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

/* pack structs the way they are on the disk */
#pragma pack(1)

typedef struct
{
	__int64					secSince1970;
	unsigned int			nanoseconds;
} BtrfsTime;

typedef struct
{
	unsigned int			crc32c;
	unsigned char			crc32c_pad			[0x1c];
	unsigned char			fsID				[0x10];
	unsigned __int64		blockNr;
	unsigned __int64		flags;

	unsigned char			chunkTreeUID		[0x10];
	unsigned __int64		generation;
	unsigned __int64		owner;
	unsigned int			nrItems;
	unsigned char			level;
} BtrfsHeader;

typedef struct
{
	unsigned __int64		objectID;
	unsigned char			type;
	unsigned __int64		offset;
} BtrfsDiskKey;

typedef struct
{
	BtrfsDiskKey			key;
	unsigned int			offset;
	unsigned int			size;
} BtrfsItem;

typedef struct
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
} BtrfsInodeItem;

typedef struct
{
	unsigned __int64		chunkSize;
	unsigned __int64		rootObjIDref;
	unsigned __int64		stripeLen;
	unsigned __int64		type;
	unsigned int			bestIOAlign;
	unsigned int			bestIOWidth;
	unsigned int			minIOSize;
	unsigned short			numStripes;
	unsigned short			subStripes;
} BtrfsChunkItem;

typedef struct
{
	unsigned __int64		devID;
	unsigned __int64		offset;
	unsigned char			devUUID				[0x10];
} BtrfsChunkItemStripe;

typedef struct
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
} BtrfsDevItem;

typedef struct
{
	unsigned int			crc32c;
	unsigned char			crc32c_pad			[0x1c];
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
	unsigned __int64		rootDirObjectID;
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
} BtrfsSuperblock;

typedef struct
{
	unsigned __int64		logiOffset;
	BtrfsChunkItem			chunkItem;
	BtrfsChunkItemStripe	*stripes;
} Chunk;
