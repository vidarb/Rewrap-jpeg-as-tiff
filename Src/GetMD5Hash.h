// File: GetMD5Hash.h
// This file is part of the project: Rewrap-jpeg-as-tiff -- Rewraps a jpeg file into a TIFF container, without loss of information (no re-encoding).
// Copyright(c) 2018 Vidar Bosnes
// This software is licenced under the GNU GENERAL PUBLIC LICENSE Version 3

#ifndef VIBO_GETMD5HASH_H_INCLUDED
#define VIBO_GETMD5HASH_H_INCLUDED

#include "Util.h"
#include <string>

namespace vibo
{
	std::wstring GetMD5Hash(const ByteVector& vec);
}


#endif