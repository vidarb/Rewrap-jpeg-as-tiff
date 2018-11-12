// File: CreateSegment.h
// This file is part of the project: Rewrap-jpeg-as-tiff -- Rewraps a jpeg file into a TIFF container, without loss of information (no re-encoding).
// Copyright(c) 2018 Vidar Bosnes
// This software is licenced under the GNU GENERAL PUBLIC LICENSE Version 3

#ifndef CREATESEGMENT_H_INCLUDED
#define CREATESEGMENT_H_INCLUDED
#pragma once

#include "Util.h"
#include <memory>
#include "FileSegment.h"

class FileSegment; // Forward declaration

std::shared_ptr<FileSegment> CreateSegment(Segmenttype seg, Endianness e, Offset_t offset, int size);
Segmenttype GetSegmenttype(const FileSegment& fs);
std::wstring GetSegmentName(const FileSegment& fs);


#endif
