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
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <Windows.h>

namespace WinBtrfsService
{
	FILE *logFile = NULL;
	HANDLE hMutex = INVALID_HANDLE_VALUE;
	
	/* note: the localtime & asctime functions are not threadsafe. */

	void logInit()
	{
		time_t now = time(NULL);
		tm *now_tm = localtime(&now);

		/* redirect stderr to a file in case a fatal error occurs (e.g. assertion failure) */
		freopen("WinBtrfsService_stderr.log", "a", stderr);
		
		logFile = fopen("WinBtrfsService.log", "a");

		/* if the log file has other contents, put in a space */
		if (ftell(logFile) > 0)
			fputs("\n\n", logFile);

		/* print the log instance header */
		fprintf(logFile, "WinBtrfsService PID: %u\nStarted on %s\n",
			GetCurrentProcessId(), asctime(now_tm));
		fflush(logFile);
		
		assert((hMutex = CreateMutex(NULL, FALSE, NULL)) != INVALID_HANDLE_VALUE);
	}

	void log(const char *format, ...)
	{
		va_list args;

		WaitForSingleObject(hMutex, INFINITE);

		time_t now = time(NULL);
		tm *now_tm = localtime(&now);

		/* add a timestamp to each entry */
		fprintf(logFile, "[%04d.%02d.%02d|%02d:%02d:%02d] ",
			now_tm->tm_year + 1900, now_tm->tm_mon + 1, now_tm->tm_mday,
			now_tm->tm_hour, now_tm->tm_min, now_tm->tm_sec);

		va_start(args, format);
		vfprintf(logFile, format, args);
		va_end(args);

		fflush(logFile);

		ReleaseMutex(hMutex);
	}
	
	void logClose()
	{
		fclose(logFile);
		CloseHandle(hMutex);
	}

	const char *getErrorMessage(DWORD error)
	{
		const char *buffer = NULL;
		
		FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, error, 0, (LPSTR)&buffer, 1024, NULL);

		assert(buffer != NULL);

		/* buffer should be freed by LocalFree, but a 1K leak that happens once isn't an issue */

		return buffer;
	}
}
