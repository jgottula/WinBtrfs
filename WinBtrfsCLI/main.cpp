/* WinBtrfsCLI/main.cpp
 * stub to invoke WinBtrfsLib
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

__declspec(dllimport) int __stdcall winBtrfsLib_main(int argc, char **argv);

int main(int argc, char **argv)
{
	LoadLibraryA("WinBtrfsLib.dll");
	winBtrfsLib_main(argc, argv);
}
