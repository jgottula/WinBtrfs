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
	HANDLE hMutex = INVALID_HANDLE_VALUE;
	
	/* note: the localtime & asctime functions are not threadsafe. */

	void logInit()
	{
		time_t now = time(NULL);
		tm *now_tm = localtime(&now);
		
		/* if the log file already has other contents, put in a space */
		FILE *logFile = fopen("WinBtrfsService_log.txt", "r+");
		if (logFile != NULL)
		{
			fputs("\n\n", logFile);
			fclose(logFile);
		}

		/* redirect stdout and stderr to files so the user can actually read them */
		freopen("WinBtrfsService_log.txt", "a", stdout);
		freopen("WinBtrfsService_stderr.txt", "a", stderr);

		/* print the log instance header */
		printf("WinBtrfsService PID: %u\nStarted on %s\n",
			GetCurrentProcessId(), asctime(now_tm));
		
		assert((hMutex = CreateMutex(NULL, FALSE, NULL)) != INVALID_HANDLE_VALUE);
	}

	void log(const char *format, ...)
	{
		va_list args;

		WaitForSingleObject(hMutex, INFINITE);

		time_t now = time(NULL);
		tm *now_tm = localtime(&now);

		/* add a timestamp to each entry */
		printf("[%04d.%02d.%02d|%02d:%02d:%02d] ",
			now_tm->tm_year + 1900, now_tm->tm_mon + 1, now_tm->tm_mday,
			now_tm->tm_hour, now_tm->tm_min, now_tm->tm_sec);

		va_start(args, format);
		vprintf(format, args);
		va_end(args);

		ReleaseMutex(hMutex);
	}
	
	void logClose()
	{
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
