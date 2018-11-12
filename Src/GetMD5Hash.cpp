// File: GetMD5Hash.cpp
// This file is part of the project: Rewrap-jpeg-as-tiff -- Rewraps a jpeg file into a TIFF container, without loss of information (no re-encoding).
// Copyright(c) 2018 Vidar Bosnes
// This software is licenced under the GNU GENERAL PUBLIC LICENSE Version 3

#include "GetMD5Hash.h"
#include "Md5.h"
#include "Exception.h"

namespace vibo
{
	std::wstring GetMD5Hash(const ByteVector& vec)
	{
		MD5_CTX ctx;
		unsigned char result[100];
		memset(&ctx, 0, sizeof(ctx));
		memset(result, 0, 100);

		MD5_Init(&ctx);
		MD5_Update(&ctx, &vec[0], static_cast<unsigned long>(vec.size())); // cast to silence error message in 64-bit build.
		MD5_Final(result, &ctx);

		static const std::wstring hexdigit(L"0123456789ABCDEF");
		std::wstring retval(L"");

		for (int i = 0; i < 16; ++i)
		{
			int ch = result[i];
			int hi = ch / 16;
			int lo = (ch % 16);
			retval += hexdigit[hi];
			retval += hexdigit[lo];
		}
		return retval;
	}
}
