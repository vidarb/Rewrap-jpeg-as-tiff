// File: ReadJpegMetadata.h
// This file is part of the project: Rewrap-jpeg-as-tiff -- Rewraps a jpeg file into a TIFF container, without loss of information (no re-encoding).
// Copyright(c) 2018 Vidar Bosnes
// This software is licenced under the GNU GENERAL PUBLIC LICENSE Version 3

#ifndef READJPEGMETADATA_H_INCLUDED
#define READJPEGMETADATA_H_INCLUDED

#include "Util.h"
#include "JpegSegments.h"
#include <memory>
#include <tuple>
#include "TiffDirEntry.h"

ByteVector ReadIccProfile(std::vector<std::shared_ptr<JpegApp2Segment>>& App2Segments);


struct exif_info
{
	Endianness endianness;
	std::vector<std::tuple<TiffDirEntry, ByteVector>> main_dir;
	std::vector<std::tuple<TiffDirEntry, ByteVector>> exif_dir;
	std::vector<std::tuple<TiffDirEntry, ByteVector>> gps_dir;
};

exif_info ReadApp1Metadata(std::vector<std::shared_ptr<JpegApp1Segment>>& App1Segments);


#endif