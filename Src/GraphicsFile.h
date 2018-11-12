// File: GraphicsFile.h
// This file is part of the project: Rewrap-jpeg-as-tiff -- Rewraps a jpeg file into a TIFF container, without loss of information (no re-encoding).
// Copyright(c) 2018 Vidar Bosnes
// This software is licenced under the GNU GENERAL PUBLIC LICENSE Version 3

#ifndef GRAPHICSFILE_H_INCLUDED
#define GRAPHICSFILE_H_INCLUDED

#include "FileSegment.h"
#include <memory>

class FileSegment; // forward decl

enum class Filetype
{
	TIFF_Little_endian,
	TIFF_Big_endian,
	JPEG,
	Unknown
};


typedef std::vector<std::shared_ptr<FileSegment>> GraphicsVector;

// --------------------------------------------------------------------------------------------------------------------
//		Free functions
// --------------------------------------------------------------------------------------------------------------------

Endianness GetEndianness(Filetype t);

Offset_t AddSegmentNopad(GraphicsVector& vec, std::shared_ptr<FileSegment> seg);
Offset_t AddSegmentPadded(GraphicsVector& vec, std::shared_ptr<FileSegment> seg);

void Dump(const GraphicsVector& vec);

#endif
