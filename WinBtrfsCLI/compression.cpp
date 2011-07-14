/* compression.cpp
 * data compression algorithms
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

#include <cassert>
#include "minilzo/minilzo.h"
#include "zlib/zlib.h"
#include "endian.h"

void lzoDecompress(const unsigned char *compressed, unsigned char *decompressed,
	unsigned __int64 cSize, unsigned __int64 dSize)
{
	lzo_uint lzoTotLen, lzoInLen, lzoOutLen, lzoBytesRead = 0, lzoBytesWritten = 0;
	
	/* must at least contain the 32-bit total size header */
	assert(cSize >= 4);

	/* total number of compressed bytes in the extent */
	lzoTotLen = endian32(*((unsigned int *)compressed));
	compressed += 4;

	while (lzoBytesRead < lzoTotLen - 4)
	{
		lzoInLen = endian32(*((unsigned int *)compressed));
		compressed += 4;

		int result = lzo1x_decompress_safe(compressed, lzoInLen,
			decompressed, &lzoOutLen, NULL);
		/* this should not always be fatal */
		assert(result == LZO_E_OK);

		lzoBytesRead += lzoInLen + 4;
		lzoBytesWritten += lzoOutLen;

		compressed += lzoInLen;
		decompressed += lzoOutLen;
	}

	assert(lzoBytesWritten <= dSize);
}

void zlibDecompress(const unsigned char *compressed, unsigned char *decompressed,
	unsigned __int64 cSize, unsigned __int64 dSize)
{
	z_stream zStream;

	zStream.zalloc = NULL;
	zStream.zfree = NULL;
	zStream.opaque = NULL;
	zStream.next_in = const_cast<unsigned char *>(compressed); // why does zlib want mutable input?
	zStream.avail_in = 0;
	zStream.next_out = decompressed;

	/* this should not always be fatal */
	assert(inflateInit(&zStream) == Z_OK);

	while (zStream.total_in < cSize && zStream.total_out < dSize)
	{
		/* look this up */
		zStream.avail_in = zStream.avail_out = 1;

		int err = inflate(&zStream, Z_NO_FLUSH);
		if (err == Z_STREAM_END)
			break;
		/* this should not always be fatal */
		assert(err == Z_OK);
	}
	
	/* this should not always be fatal */
	assert(inflateEnd(&zStream) == Z_OK);
}
