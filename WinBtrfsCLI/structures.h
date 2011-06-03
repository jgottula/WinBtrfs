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

typedef struct
{
	unsigned __int8 csum[32];
	unsigned __int8 fsID[16];
	unsigned __int64 blockNr;
	unsigned __int64 flags;

	unsigned __int8 chunkTreeUID[16];
	unsigned __int64 generation;
	unsigned __int64 owner;
	unsigned __int32 nrItems;
	unsigned __int8 level;
} BtrfsHeader;

typedef struct
{
	unsigned __int64 objectID;
	unsigned __int8 type;
	unsigned __int64 offset;
} BtrfsDiskKey;

typedef struct
{
	BtrfsDiskKey key;
	unsigned __int32 offset;
	unsigned __int32 size;
} BtrfsItem;

typedef struct
{
	unsigned __int64 chunkSize;
	unsigned __int64 rootObjIDref;
	unsigned __int64 stripeLen;
	unsigned __int64 type;
	unsigned __int32 ioAlign;
	unsigned __int32 ioWidth;
	unsigned __int32 sectorSize;
	unsigned __int16 numStripes;
	unsigned __int16 subStripes;
} BtrfsChunkItem;

typedef struct
{
	unsigned __int64 devID;
	unsigned __int64 offset;
	unsigned __int8 devUUID[0x10];
} BtrfsChunkItemStripe;

typedef struct
{
	unsigned __int32	crc32;
	unsigned __int8		crc32_pad			[0x1c];
	unsigned __int8		uuid				[0x10];
	unsigned __int64	physAddr;
	unsigned __int64	flags;
	char				magic				[0x08];
	unsigned __int64	generation;
	unsigned __int64	rootTreeLAddr;
	unsigned __int64	chunkTreeLAddr;
	unsigned __int64	logTreeLAddr;
	unsigned __int64	logRootTransID;
	unsigned __int64	totalBytes;
	unsigned __int64	bytesUsed;
	unsigned __int64	rootDirObjectID;
	unsigned __int64	numDevices;
	unsigned __int32	sectorSize;
	unsigned __int32	nodeSize;
	unsigned __int32	leafSize;
	unsigned __int32	stripeSize;
	unsigned __int32	n;
	unsigned __int64	chunkRootGeneration;
	unsigned __int64	compatFlags;
	unsigned __int64	compatROFlags;
	unsigned __int64	incompatFlags;
	unsigned __int16	csumType;
	unsigned __int8		rootLevel;
	unsigned __int8		chunkRootLevel;
	unsigned __int8		logRootLevel;
	unsigned __int8		deviceData			[0x62];
	char				label				[0x100];
	unsigned __int8		reserved			[0x100];
	unsigned __int8		chunks				[0x800];
	unsigned __int8		unused				[0x4d5];
} Superblock;
