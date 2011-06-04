/* endian.h
 * functions to deal with disk/CPU endianness differences
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

void endianDetect();
unsigned short endian16(unsigned short leFromDisk);
unsigned int endian32(unsigned int leFromDisk);
unsigned __int64 endian64(unsigned __int64 leFromDisk);
