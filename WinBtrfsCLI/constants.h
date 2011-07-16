/* WinBtrfsCLI/constants.h
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

#ifndef WINBTRFS_CONSTANTS_H
#define WINBTRFS_CONSTANTS_H

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

#endif
