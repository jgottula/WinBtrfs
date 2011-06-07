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

/* Btrfs objectIDs */
#define OBJID_ROOT_TREE			0x01
#define OBJID_EXTENT_TREE		0x02
#define OBJID_CHUNK_TREE		0x03
#define OBJID_DEV_TREE			0x04
#define OBJID_FS_TREE			0x05
#define OBJID_ROOT_TREE_DIR		0x06
#define OBJID_CSUM_TREE			0x07
#define OBJID_ROOT_DIR			0x100
#define OBJID_ORPHAN			-0x05
#define OBJID_TREE_LOG			-0x06
#define OBJID_TREE_LOG_FIXUP	-0x07
#define OBJID_TREE_RELOC		-0x08
#define OBJID_DATA_RELOC_TREE	-0x09
#define OBJID_EXTENT_CSUM		-0x0a
#define OBJID_MULTIPLE			-0x100

/* Btrfs item types */
#define TYPE_INODE_ITEM			0x01
#define TYPE_INODE_REF			0x0c
#define TYPE_XATTR_ITEM			0x18
#define TYPE_ORPHAN_ITEM		0x30
#define TYPE_DIR_LOG_ITEM		0x3c
#define TYPE_DIR_LOG_INDEX		0x48
#define TYPE_DIR_ITEM			0x54
#define TYPE_DIR_INDEX			0x60
#define TYPE_EXTENT_DATA		0x6c
#define TYPE_EXTENT_CSUM		0x80
#define TYPE_ROOT_ITEM			0x84
#define TYPE_ROOT_BACKREF		0x90
#define TYPE_ROOT_REF			0x9c
#define TYPE_EXTENT_ITEM		0xa8
#define TYPE_TREE_BLOCK_REF		0xb0
#define TYPE_EXTENT_DATA_REF	0xb2
#define TYPE_EXTENT_REF_V0		0xb4
#define TYPE_SHARED_BLOCK_REF	0xb6
#define TYPE_SHARED_DATA_REF	0xb8
#define TYPE_BLOCK_GROUP_ITEM	0xc0
#define TYPE_DEV_EXTENT			0xcc
#define TYPE_DEV_ITEM			0xd8
#define TYPE_CHUNK_ITEM			0xe4
#define TYPE_STRING_ITEM		0xfd

/* Btrfs superblock physical addresses */
#define SUPERBLOCK_1_PADDR		0x10000
#define SUPERBLOCK_2_PADDR		0x4000000
#define SUPERBLOCK_3_PADDR		0x4000000000
#define SUPERBLOCK_4_PADDR		0x4000000000000

/* address types */
#define ADDR_PHYSICAL			0x00
#define ADDR_LOGICAL			0x01

/* FS tree parser operations */
#define FSOP_NAME_TO_ID			0x00
#define FSOP_ID_TO_INODE		0x01
#define FSOP_ID_TO_CHILD_IDS	0x02
#define FSOP_ID_TO_NAME			0x03
#define FSOP_ID_TO_PARENT_ID	0x04

/* stat mode constants */
#define S_IFMT					0170000	// bitmask for file type fields
#define S_IFSOCK				0140000	// socket
#define S_IFLNK					0120000	// symbolic link
#define S_IFREG					0100000	// regular file
#define S_IFBLK					0060000	// block device
#define S_IFDIR					0040000	// directory
#define S_IFCHR					0020000	// character device
#define S_IFIFO					0010000	// FIFO
#define S_ISUID					0004000	// set UID bit
#define S_ISGID					0002000	// set GID bit
#define S_ISVTX					0001000	// sticky bit
#define S_IRWXU					00700	// mask for owner's permissions
#define S_IRUSR					00400	// owner has read permission
#define S_IWUSR					00200	// owner has write permission
#define S_IXUSR					00100	// owner has execute permission
#define S_IRWXG					00070	// mask for group's permissions
#define S_IRGRP					00040	// group has read permission
#define S_IWGRP					00020	// group has write permission
#define S_IXGRP					00010	// group has execute permission
#define S_IRWXO					00007	// mask for others' permissions
#define S_IROTH					00004	// others have read permission
#define S_IWOTH					00002	// others have write permission
#define S_IXOTH					00001	// others have execute permission
