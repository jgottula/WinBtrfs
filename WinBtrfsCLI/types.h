/* types.h
 * user type and struct definitions
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

#ifndef WINBTRFS_TYPES_H
#define WINBTRFS_TYPES_H

/* pack structs the way they are on the disk */
#pragma pack(1)

enum BtrfsObjID : unsigned __int64
{
	OBJID_ROOT_TREE = 0x01,
	OBJID_EXTENT_TREE = 0x02,
	OBJID_CHUNK_TREE = 0x03,
	OBJID_DEV_TREE = 0x04,
	OBJID_FS_TREE = 0x05,
	OBJID_ROOT_TREE_DIR = 0x06,
	OBJID_CSUM_TREE = 0x07,
	OBJID_ROOT_DIR = 0x100,
	OBJID_ORPHAN = (unsigned __int64)-0x05,
	OBJID_TREE_LOG = (unsigned __int64)-0x06,
	OBJID_TREE_LOG_FIXUP = (unsigned __int64)-0x07,
	OBJID_TREE_RELOC = (unsigned __int64)-0x08,
	OBJID_DATA_RELOC_TREE = (unsigned __int64)-0x09,
	OBJID_EXTENT_CSUM = (unsigned __int64)-0x0a,
	OBJID_MULTIPLE = (unsigned __int64)-0x100
};


enum BtrfsItemType : unsigned char
{
	TYPE_INODE_ITEM = 0x01,
	TYPE_INODE_REF = 0x0c,
	TYPE_XATTR_ITEM = 0x18,
	TYPE_ORPHAN_ITEM = 0x30,
	TYPE_DIR_LOG_ITEM = 0x3c,
	TYPE_DIR_LOG_INDEX = 0x48,
	TYPE_DIR_ITEM = 0x54,
	TYPE_DIR_INDEX = 0x60,
	TYPE_EXTENT_DATA = 0x6c,
	TYPE_EXTENT_CSUM = 0x80,
	TYPE_ROOT_ITEM = 0x84,
	TYPE_ROOT_BACKREF = 0x90,
	TYPE_ROOT_REF = 0x9c,
	TYPE_EXTENT_ITEM = 0xa8,
	TYPE_TREE_BLOCK_REF = 0xb0,
	TYPE_EXTENT_DATA_REF = 0xb2,
	TYPE_EXTENT_REF_V0 = 0xb4,
	TYPE_SHARED_BLOCK_REF = 0xb6,
	TYPE_SHARED_DATA_REF = 0xb8,
	TYPE_BLOCK_GROUP_ITEM = 0xc0,
	TYPE_DEV_EXTENT = 0xcc,
	TYPE_DEV_ITEM = 0xd8,
	TYPE_CHUNK_ITEM = 0xe4,
	TYPE_STRING_ITEM = 0xfd
};

enum BlockGroupFlags : unsigned __int64
{
	BGFLAG_DATA = 0x1,
	BGFLAG_SYSTEM = 0x2,
	BGFLAG_METADATA = 0x4,
	BGFLAG_RAID0 = 0x8,
	BGFLAG_RAID1 = 0x10,
	BGFLAG_DUPLICATE = 0x20,
	BGFLAG_RAID10 = 0x40
};

enum FileDataType : unsigned char
{
	FILEDATA_INLINE = 0,
	FILEDATA_REGULAR = 1,
	FILEDATA_PREALLOC = 2
};

enum CompressionType : unsigned char
{
	COMPRESSION_NONE = 0,
	COMPRESSION_ZLIB = 1,
	COMPRESSION_LZO = 2
};

enum EncryptionType : unsigned char
{
	ENCRYPTION_NONE = 0
};

enum EncodingType : unsigned char
{
	ENCODING_NONE = 0
};

enum AddrType
{
	ADDR_PHYSICAL,
	ADDR_LOGICAL
};

/* chunk tree operations */
enum CTOperation
{
	CTOP_LOAD,
	CTOP_DUMP_TREE
};

/* root tree operations */
enum RTOperation
{
	RTOP_LOAD,
	RTOP_DUMP_TREE,
	RTOP_DEFAULT_SUBVOL,
	RTOP_GET_SUBVOL_ID,
	RTOP_SUBVOL_EXISTS,
	RTOP_GET_ADDR
};

/* FS tree operations */
enum FSOperation
{
	FSOP_NAME_TO_ID,
	FSOP_DUMP_TREE,
	FSOP_GET_FILE_PKG,
	FSOP_DIR_LIST,
	FSOP_GET_INODE
};

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
	BlockGroupFlags			type;
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
	unsigned char			fsUUID				[0x10];
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

struct KeyedItem
{
	BtrfsDiskKey			key;
	void					*data;
};

struct Root
{
	BtrfsObjID				objectID;
	BtrfsRootItem			rootItem;
};

struct FileID
{
	BtrfsObjID				treeID;
	BtrfsObjID				objectID;
};

struct FilePkg
{
	FileID					fileID;
	FileID					parentID;		// unused in dirlist functions
	BtrfsInodeItem			inode;
	BtrfsInodeItem			parentInode;	// unused in dirlist functions
	size_t					numExtents;		// unused in dirlist functions
	KeyedItem				*extents;		// unused in dirlist functions
	char					name[256];
	bool					hidden;
};

struct DirList
{
	size_t					numEntries;
	FilePkg					*entries;
};

/* size checks for on-disk types and structs */
static_assert(sizeof(BtrfsObjID) == sizeof(unsigned __int64), "BtrfsObjID has an unexpected size!");
static_assert(sizeof(BtrfsItemType) == sizeof(unsigned char), "BtrfsItemType has an unexpected size!");
static_assert(sizeof(BlockGroupFlags) == sizeof(unsigned __int64), "BlockGroupFlags has an unexpected size!");
static_assert(sizeof(FileDataType) == sizeof(unsigned char), "FileDataType has an unexpected size!");
static_assert(sizeof(CompressionType) == sizeof(unsigned char), "CompressionType has an unexpected size!");
static_assert(sizeof(EncryptionType) == sizeof(unsigned char), "EncryptionType has an unexpected size!");
static_assert(sizeof(EncodingType) == sizeof(unsigned char), "EncodingType has an unexpected size!");
static_assert(sizeof(BtrfsTime) == 0x0c, "BtrfsTime has an unexpected size!");
static_assert(sizeof(BtrfsHeader) == 0x65, "BtrfsHeader has an unexpected size!");
static_assert(sizeof(BtrfsDiskKey) == 0x11, "BtrfsDiskKey has an unexpected size!");
static_assert(sizeof(BtrfsKeyPtr) == 0x21, "BtrfsKeyPtr has an unexpected size!");
static_assert(sizeof(BtrfsItem) == 0x19, "BtrfsItem has an unexpected size!");
static_assert(sizeof(BtrfsInodeItem) == 0xa0, "BtrfsInodeItem has an unexpected size!");
static_assert(sizeof(BtrfsInodeRef) == 0x0a, "BtrfsInodeRef has an unexpected size!");
static_assert(sizeof(BtrfsDirItem) == 0x1e, "BtrfsDirItem has an unexpected size!");
static_assert(sizeof(BtrfsDirIndex) == 0x1e, "BtrfsDirIndex has an unexpected size!");
static_assert(sizeof(BtrfsExtentData) == 0x15, "BtrfsExtentData has an unexpected size!");
static_assert(sizeof(BtrfsExtentDataNonInline) == 0x20, "BtrfsExtentDataNonInline has an unexpected size!");
static_assert(sizeof(BtrfsRootItem) == 0xef, "BtrfsRootItem has an unexpected size!");
static_assert(sizeof(BtrfsRootBackref) == 0x12, "BtrfsRootBackref has an unexpected size!");
static_assert(sizeof(BtrfsRootRef) == 0x12, "BtrfsRootRef has an unexpected size!");
static_assert(sizeof(BtrfsChunkItem) == 0x30, "BtrfsChunkItem has an unexpected size!");
static_assert(sizeof(BtrfsChunkItemStripe) == 0x20, "BtrfsChunkItemStripe has an unexpected size!");
static_assert(sizeof(BtrfsDevItem) == 0x62, "BtrfsDevItem has an unexpected size!");
static_assert(sizeof(BtrfsSBChunk) == 0x41, "BtrfsSBChunk has an unexpected size!");
static_assert(sizeof(BtrfsSuperblock) == 0x1000, "BtrfsSuperblock has an unexpected size!");

#endif
