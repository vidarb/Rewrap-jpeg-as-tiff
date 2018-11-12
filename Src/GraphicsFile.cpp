// File: GraphicsFile.cpp
// This file is part of the project: Rewrap-jpeg-as-tiff -- Rewraps a jpeg file into a TIFF container, without loss of information (no re-encoding).
// Copyright(c) 2018 Vidar Bosnes
// This software is licenced under the GNU GENERAL PUBLIC LICENSE Version 3

#include "GraphicsFile.h"
#include "Exception.h"
#include "FileSegment.h"
#include "CreateSegment.h"

Offset_t AddSegmentNopad(GraphicsVector& vec, std::shared_ptr<FileSegment> seg)
{
	vec.push_back(seg);
	return vec.back()->GetOffset() + vec.back()->GetSize();
}


Offset_t AddSegmentPadded(GraphicsVector& vec, std::shared_ptr<FileSegment> seg)
{
	Endianness e = seg->FileEndianness();
	vec.push_back(seg);
	Offset_t next_offset = vec.back()->GetOffset() + vec.back()->GetSize();

	int pad_bytes = next_offset % 2;
	if (pad_bytes != 0)
	{
		std::shared_ptr<FileSegment> S = CreateSegment(Segmenttype::Padding, e, next_offset, pad_bytes);
		vec.push_back(S);
	}
	return vec.back()->GetOffset() + vec.back()->GetSize();
}


void Dump(const GraphicsVector& vec)
{
	for (auto it = vec.cbegin(); it != vec.cend(); ++it)
	{
		(*it)->Dump();
	}
}


// --------------------------------------------------------------------------------------------------------------------
//		Free function: GetEndianness()
// --------------------------------------------------------------------------------------------------------------------

Endianness GetEndianness(Filetype t)
{
	switch (t)
	{
	case Filetype::JPEG: return Endianness::Big;
	case Filetype::TIFF_Big_endian: return Endianness::Big;
	case Filetype::TIFF_Little_endian: return Endianness::Little;
	}
	ASSERT(false);
	return Endianness::Little; // Whatever, never reached.
}

