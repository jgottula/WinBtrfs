/* WinBtrfsService/log.h
 * logging functionality
 *
 * WinBtrfs
 * Copyright (c) 2011 Justin Gottula
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

namespace WinBtrfsService
{
	void logInit();
	void log(const char *format, ...);
	void logClose();
}
