// File: JpegSegments.h
// This file is part of the project: Rewrap-jpeg-as-tiff -- Rewraps a jpeg file into a TIFF container, without loss of information (no re-encoding).
// Copyright(c) 2018 Vidar Bosnes
// This software is licenced under the GNU GENERAL PUBLIC LICENSE Version 3

#ifndef JPEGSEGMENTS_H_INCLUDED
#define JPEGSEGMENTS_H_INCLUDED

#include "GraphicsFile.h"
#include "FileSegment.h"
#include <stdio.h>

// --------------------------------------------------------------------------------------------------------------------
//		Free functions
// --------------------------------------------------------------------------------------------------------------------

void ReadJpegStartOfImage(FILE* f, GraphicsVector& G, Offset_t offset, const std::wstring& comment);
void ReadJpegEndOfImage(FILE* f, GraphicsVector& G, Offset_t offset, const std::wstring& comment);
void ReadJpegRestartMarker(FILE* f, GraphicsVector& G, Offset_t offset);
void ReadJpegUnspecifiedSegment(FILE* f, GraphicsVector& G, Segmenttype seg, Offset_t offset);
void ReadJpegImagedata(FILE* f, GraphicsVector& G);
void ReadJpegFileOrEmbeddedSection(FILE* f, GraphicsVector& G, Offset_t offset, int datasize, const std::wstring& comment);

// --------------------------------------------------------------------------------------------------------------------
//		Derived JPEG classes
// --------------------------------------------------------------------------------------------------------------------

class JpegSegment : public FileSegment
{
public:
	JpegSegment(Offset_t offset, int size, Endianness e);
	Endianness FileEndianness() const;

protected:
	void InterpretData() override;
	~JpegSegment() = default;
};


class JpegStartOfImage : public JpegSegment
{
public:
	JpegStartOfImage(Offset_t offset, int size, Endianness e);
	~JpegStartOfImage() = default;
	void RebuildBinaryData();
};


class JpegEndOfImage : public JpegSegment
{
public:
	JpegEndOfImage(Offset_t offset, int size, Endianness e); // fixed size 2
	~JpegEndOfImage() = default;
	void RebuildBinaryData();
};


class JpegRestartMarker : public JpegSegment
{
public:
	JpegRestartMarker(Offset_t offset, int size, Endianness e);
	~JpegRestartMarker() = default;
};


class JpegApp0Segment : public JpegSegment
{
public:
	JpegApp0Segment(Offset_t offset, int size, Endianness e);
	~JpegApp0Segment() = default;
};


class JpegApp1Segment : public JpegSegment
{
public:
	JpegApp1Segment(Offset_t offset, int size, Endianness e);
	~JpegApp1Segment() = default;
};


class JpegApp2Segment : public JpegSegment
{
public:
	JpegApp2Segment(Offset_t offset, int size, Endianness e);
	~JpegApp2Segment() = default;
};


class JpegApp3Segment : public JpegSegment
{
public:
	JpegApp3Segment(Offset_t offset, int size, Endianness e);
	~JpegApp3Segment() = default;
};


class JpegOtherAppSegment : public JpegSegment
{
public:
	JpegOtherAppSegment(Offset_t offset, int size, Endianness e);
	~JpegOtherAppSegment() = default;
};


class JpegQuantizationTable : public JpegSegment
{
public:
	JpegQuantizationTable(Offset_t offset, int size, Endianness e);
	~JpegQuantizationTable() = default;
};


struct Component_info // Used by JpegStartOfFrame
{
	int id;
	int sampling_factors;
	int quantitation_table_number;
};


class JpegStartOfFrame : public JpegSegment
{
	int m_precision;
	int m_width;
	int m_length;
	int m_num_components;
	std::vector<Component_info> m_component_info;

public:
	JpegStartOfFrame(Offset_t offset, int size, Endianness e);
	~JpegStartOfFrame() = default;
	std::vector<std::wstring> StringRepresentation() const override;

	// Access methods
	int GetPrecision() const;
	int GetImageWidth() const;
	int GetImageLength() const;
	int GetNumComponents() const;
	int GetHorizontalSamplingFactor(unsigned component) const;
	int GetVerticalSamplingFactor(unsigned component) const;

protected:
	void InterpretData() override;
};


class JpegHuffmanTable : public JpegSegment
{
public:
	JpegHuffmanTable(Offset_t offset, int size, Endianness e);
	~JpegHuffmanTable() = default;
};


class JpegStartOfScan : public JpegSegment
{
public:
	JpegStartOfScan(Offset_t offset, int size, Endianness e);
	~JpegStartOfScan() = default;
};


class JpegImageData : public JpegSegment
{
public:
	JpegImageData(Offset_t offset, int size, Endianness e);
	~JpegImageData() = default;
};


class JpegNumberOfLines : public JpegSegment
{
public:
	JpegNumberOfLines(Offset_t offset, int size, Endianness e);
	~JpegNumberOfLines() = default;
};


class JpegRestartInterval : public JpegSegment
{
public:
	JpegRestartInterval(Offset_t offset, int size, Endianness e);
	~JpegRestartInterval() = default;
};


class JpegSpecialSegment : public JpegSegment
{
public:
	JpegSpecialSegment(Offset_t offset, int size, Endianness e);
	~JpegSpecialSegment() = default;
};


class JpegCommentSegment : public JpegSegment
{
public:
	JpegCommentSegment(Offset_t offset, int size, Endianness e);
	~JpegCommentSegment() = default;
};


class JpegReservedSegment : public JpegSegment
{
public:
	JpegReservedSegment(Offset_t offset, int size, Endianness e);
	~JpegReservedSegment() = default;
};


class JpegUnknownSegment : public JpegSegment
{
public:
	JpegUnknownSegment(Offset_t offset, int size, Endianness e);
	~JpegUnknownSegment() = default;
};

#endif

