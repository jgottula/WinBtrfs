/* WinBtrfsService/log.cpp
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

#include "log.h"
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <Windows.h>

namespace WinBtrfsService
{
	FILE *logFile = NULL;
	
	/* note: the localtime & asctime functions are not threadsafe. */

	void logInit()
	{
		time_t now = time(NULL);
		tm *now_tm = localtime(&now);
		
		logFile = fopen("WinBtrfsService.log", "a+");
		fprintf(logFile, "WinBtrfsService PID: %u\nStarted on %s\n",
			GetCurrentProcessId(), asctime(now_tm));
	}

	void log(const char *format, ...)
	{
		va_list args;
		time_t now = time(NULL);
		tm *now_tm = localtime(&now);

		/* add a timestamp to each entry */
		fprintf(logFile, "[%04d.%02d.%02d|%02d:%02d:%02d] ",
			now_tm->tm_year + 1900, now_tm->tm_mon + 1, now_tm->tm_mday,
			now_tm->tm_hour, now_tm->tm_min, now_tm->tm_sec);

		va_start(args, format);
		vfprintf(logFile, format, args);
		va_end(args);
	}

	void logClose()
	{
		fputs("\n\n", logFile);
		fclose(logFile);
	}
}
