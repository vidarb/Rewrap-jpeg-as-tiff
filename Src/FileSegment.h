// File: FileSegment.h
// This file is part of the project: Rewrap-jpeg-as-tiff -- Rewraps a jpeg file into a TIFF container, without loss of information (no re-encoding).
// Copyright(c) 2018 Vidar Bosnes
// This software is licenced under the GNU GENERAL PUBLIC LICENSE Version 3

#ifndef FILESEGMENT_H_INCLUDED
#define FILESEGMENT_H_INCLUDED

#include <stdint.h>
#include <string>
#include "Util.h"
#include <memory>

enum class Segmenttype
{
	Undefined,				// Illegal value
	Padding,				// Used to force alignment to two-byte boundaries
	JpegStartOfImage,		// ff d8
	JpegEndOfImage,			// ff f9 NO DATA
	JpegRestartMarker,		// ff d0- ff d7 NO DATA

	JpegApp0Segment,		// JFIF header
	JpegApp1Segment,        // EXIF header, XMP
	JpegApp2Segment,        // Usually ICC profile
	JpegApp3Segment,        // Metadata
	JpegOtherAppSegment,

	JpegQuantizationTable,	// ff db
	JpegStartOfFrame,		// ff c0 m.m.
	JpegHuffmanTable,		// ff c4
	JpegStartOfScan,		// ff da
	JpegImageData,			// no ID, follows StartOfScan

	JpegNumberOfLines,
	JpegRestartInterval,	// ff dd
	JpegSpecialSegment,
	JpegCommentSegment,
	JpegReservedSegment,	// c8, 02 -- bf, f0--fd
	JpegUnknownSegment,		// ff xx

	TiffHeader,             // 4 bytes starting with MM or II plus 4-byte offset to first directory
	TiffDirectory,          // Variable size
	TiffByteVector,
	TiffUShortVector,
	TiffOffsetTable,
	TiffBytecountTable,
	TiffImageData
};


// --------------------------------------------------------------------------------------------------------------------
//		Base class FileSegment
// --------------------------------------------------------------------------------------------------------------------

class FileSegment
{
protected:
	Offset_t     m_offset;
	ULong_t      m_size;
	ByteVector   m_data;
	std::wstring m_label;

public:
	FileSegment(int offset, int size);
	int GetSize() const;
	virtual void RebuildBinaryData(); // Create m_data from scratch. Only defined when needed; otherwise throws exceptrion.

	int GetOffset() const;
	void SetOffset(int offset);

	void Dump() const;
	virtual std::vector<std::wstring> StringRepresentation() const;

	int GetDataByte(int) const;
	Segmenttype GetSegmenttype() const;
	virtual Endianness FileEndianness() const = 0;

	// Label -- For dumping purposes
	void SetLabel(const std::wstring&);
	std::wstring Label() const;
	bool HasLabel() const;

	// Read from file
	void ReadData(FILE* f);

	// Clone
	std::shared_ptr<FileSegment> Clone();

	// Write to disk
	void WriteToFile(FILE* f) const;

	// Copying of data
	const ByteVector& Data() const;

	virtual ~FileSegment() = default; // Should have been protected, but shared_ptr<FileSegment> fails to compile when dtor is protected.

protected:
	virtual void InterpretData(); // Interpret m_data, i.e. initialize member variables from the information in m_data. Called from ReadData() and Clone().

private: // Disallowed
	FileSegment() = delete;
	FileSegment(const FileSegment&) = delete;
	FileSegment& operator=(const FileSegment&) = delete;
};


class Padding : public FileSegment
{
	Endianness m_Endianness;

public:
	Padding(int offset, int size, Endianness e);
	Endianness FileEndianness() const override;
};



#endif

