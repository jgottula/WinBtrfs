/* btrfs_operations.h
 * high-level filesystem operations
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

void validatePath(const char *input, char *output);
int componentizePath(const char *path, char ***output);
int getPathID(const char *path, BtrfsObjID *output);
/*int getInode(BtrfsObjID objectID, Inode *output, int checkHidden);
int getName(BtrfsObjID objectID, char **output);
int dirList(BtrfsObjID objectID, DirList *output);*/
void convertTime(BtrfsTime *bTime, PFILETIME wTime);
