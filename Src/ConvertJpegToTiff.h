// File: ConvertJpegToTiff.h
// This file is part of the project: Rewrap-jpeg-as-tiff -- Rewraps a jpeg file into a TIFF container, without loss of information (no re-encoding).
// Copyright(c) 2018 Vidar Bosnes
// This software is licenced under the GNU GENERAL PUBLIC LICENSE Version 3

#ifndef CONVERTJPEGTOTIFF_H_INCLUDED
#define CONVERTJPEGTOTIFF_H_INCLUDED

#include "GraphicsFile.h"

void ConvertGraphicsFileToTiff(GraphicsVector& G, std::wstring& outfilename);


#endif
