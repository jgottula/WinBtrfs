/* WinBtrfsDrv/crc32c.h
 * crc32c checksum function
 *
 * WinBtrfs
 * Copyright (c) 2011 Justin Gottula
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#ifndef WINBTRFSDRV_CRC32_H
#define WINBTRFSDRV_CRC32_H

unsigned int crc32c(unsigned int crc, const unsigned char *data, unsigned int length);

#endif
