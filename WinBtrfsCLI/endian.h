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

#include <boost/detail/endian.hpp>

#ifndef BOOST_LITTLE_ENDIAN
#ifndef BOOST_BIG_ENDIAN
#error Architecture appears to be neither little- nor big-endian! Check endian.h!
#endif
#endif

inline unsigned short endian16(unsigned short _leFromDisk)
{
#ifdef BOOST_LITTLE_ENDIAN
	return _leFromDisk;
#elif BOOST_BIG_ENDIAN
	return _byteswap_ushort(_leFromDisk);
#endif
}

inline unsigned int endian32(unsigned int _leFromDisk)
{
#ifdef BOOST_LITTLE_ENDIAN
	return _leFromDisk;
#elif BOOST_BIG_ENDIAN
	return _byteswap_ulong(_leFromDisk);
#endif
}

inline unsigned __int64 endian64(unsigned __int64 _leFromDisk)
{
#ifdef BOOST_LITTLE_ENDIAN
	return _leFromDisk;
#elif BOOST_BIG_ENDIAN
	return _byteswap_uint64(_leFromDisk);
#endif
}
