/* util.h
 * utility functions
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

#include <Windows.h>
#include "structures.h"

void convertTime(BtrfsTime *bTime, PFILETIME wTime);
void hexToChar(unsigned char hex, char *chr);
void uuidToStr(const unsigned char *uuid, char *dest);
void stModeToStr(unsigned int mode, char *dest);
void bgFlagsToStr(BlockGroupFlags flags, char *dest);
