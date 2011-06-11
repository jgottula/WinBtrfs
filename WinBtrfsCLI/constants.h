/* constants.h
 * symbolic constant definitions
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

enum AddrType
{
	ADDR_PHYSICAL,
	ADDR_LOGICAL
};

enum FSOperation
{
	FSOP_NAME_TO_ID,
	FSOP_ID_TO_INODE,
	FSOP_ID_TO_CHILD_IDS,
	FSOP_ID_TO_NAME,
	FSOP_ID_TO_PARENT_ID,
	FSOP_DUMP_TREE
};

/* Btrfs superblock physical addresses */
const unsigned __int64 SUPERBLOCK_1_PADDR = 0x10000;
const unsigned __int64 SUPERBLOCK_2_PADDR = 0x4000000;
const unsigned __int64 SUPERBLOCK_3_PADDR = 0x4000000000;
const unsigned __int64 SUPERBLOCK_4_PADDR = 0x4000000000000;

/* stat mode constants */
const unsigned int S_IFMT = 0170000;	// bitmask for file type fields
const unsigned int S_IFSOCK = 0140000;	// socket
const unsigned int S_IFLNK = 0120000;	// symbolic link
const unsigned int S_IFREG = 0100000;	// regular file
const unsigned int S_IFBLK = 0060000;	// block device
const unsigned int S_IFDIR = 0040000;	// directory
const unsigned int S_IFCHR = 0020000;	// character device
const unsigned int S_IFIFO = 0010000;	// FIFO
const unsigned int S_ISUID = 0004000;	// set UID bit
const unsigned int S_ISGID = 0002000;	// set GID bit
const unsigned int S_ISVTX = 0001000;	// sticky bit
const unsigned int S_IRWXU = 00700;		// mask for owner's permissions
const unsigned int S_IRUSR = 00400;		// owner has read permission
const unsigned int S_IWUSR = 00200;		// owner has write permission
const unsigned int S_IXUSR = 00100;		// owner has execute permission
const unsigned int S_IRWXG = 00070;		// mask for group's permissions
const unsigned int S_IRGRP = 00040;		// group has read permission
const unsigned int S_IWGRP = 00020;		// group has write permission
const unsigned int S_IXGRP = 00010;		// group has execute permission
const unsigned int S_IRWXO = 00007;		// mask for others' permissions
const unsigned int S_IROTH = 00004;		// others have read permission
const unsigned int S_IWOTH = 00002;		// others have write permission
const unsigned int S_IXOTH = 00001;		// others have execute permission
