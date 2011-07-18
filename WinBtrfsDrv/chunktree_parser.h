/* WinBtrfsDrv/chunktree_parser.h
 * chunk tree parser
 *
 * WinBtrfs
 * Copyright (c) 2011 Justin Gottula
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#ifndef WINBTRFSDRV_CHUNKTREE_PARSER_H
#define WINBTRFSDRV_CHUNKTREE_PARSER_H

#include <vector>
#include "types.h"

namespace WinBtrfsDrv
{
	extern std::vector<KeyedItem> chunkTree;
	
	void parseChunkTree(CTOperation operation);
}

#endif
