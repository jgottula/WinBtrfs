/* WinBtrfsDrv/main.cpp
 * filesystem instance startup code
 *
 * WinBtrfs
 * Copyright (c) 2011 Justin Gottula
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <cstdio>
#include <Windows.h>

namespace WinBtrfsDrv
{
	void improperInvocation()
	{
		fprintf(stderr, "WinBtrfsDrv must be run by WinBtrfsService.\n");
		exit(1);
	}
}

using namespace WinBtrfsDrv;

int main(int argc, char **argv)
{
	wchar_t *env = GetEnvironmentStrings();

	if (memcmp(L"WinBtrfsService=1\0NamedPipe=\\\\.\\pipe\\WinBtrfsService\0",
		env, 54 * sizeof(wchar_t)) != 0)
		improperInvocation();

	FreeEnvironmentStrings(env);

	return 0;
}
