// File: JpegSegments.cpp
// This file is part of the project: Rewrap-jpeg-as-tiff -- Rewraps a jpeg file into a TIFF container, without loss of information (no re-encoding).
// Copyright(c) 2018 Vidar Bosnes
// This software is licenced under the GNU GENERAL PUBLIC LICENSE Version 3

#include "JpegSegments.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <sstream>
#include "Exception.h"
#include "CreateSegment.h"
#include "Util.h"


std::wstring JpegMarkerString(const ByteVector& vec);

// --------------------------------------------------------------------------------------------------------------------
//		class JpegSegment
// --------------------------------------------------------------------------------------------------------------------


JpegSegment::JpegSegment(Offset_t offset, int size, Endianness) : FileSegment(offset, size)
{
}

Endianness JpegSegment::FileEndianness() const
{
	return Endianness::Big;
}

void JpegSegment::InterpretData()
{
	SetLabel(JpegMarkerString(m_data));
}


// --------------------------------------------------------------------------------------------------------------------
//		class JpegStartOfImage
// --------------------------------------------------------------------------------------------------------------------

JpegStartOfImage::JpegStartOfImage(Offset_t offset, int size, Endianness e) : JpegSegment(offset, size, e) // Fixed size 2
{
}

void JpegStartOfImage::RebuildBinaryData()
{
	ASSERT(m_size == 2);
	m_data = ByteVector{ 0xff, 0xd8 };
}


// --------------------------------------------------------------------------------------------------------------------
//		class JpegEndOfImage
// --------------------------------------------------------------------------------------------------------------------

JpegEndOfImage::JpegEndOfImage(Offset_t offset, int size, Endianness e) : JpegSegment(offset, size, e) // Fixed size 2
{
}

void JpegEndOfImage::RebuildBinaryData()
{
	ASSERT(m_size == 2);
	m_data = ByteVector{ 0xff, 0xd9 };
}


// --------------------------------------------------------------------------------------------------------------------
//		class JpegRestartMarker
// --------------------------------------------------------------------------------------------------------------------

JpegRestartMarker::JpegRestartMarker(Offset_t offset, int size, Endianness e) : JpegSegment(offset, size, e)
{
}

// --------------------------------------------------------------------------------------------------------------------
//		class JpegApp0Segment -- JFIF header
// --------------------------------------------------------------------------------------------------------------------

JpegApp0Segment::JpegApp0Segment(Offset_t offset, int size, Endianness e) : JpegSegment(offset, size, e)
{
}

// --------------------------------------------------------------------------------------------------------------------
//		class JpegApp1Segment -- // EXIF header, XMP
// --------------------------------------------------------------------------------------------------------------------

JpegApp1Segment::JpegApp1Segment(Offset_t offset, int size, Endianness e) : JpegSegment(offset, size, e)
{
}

// --------------------------------------------------------------------------------------------------------------------
//		class JpegApp2Segment -- // Usually ICC profile
// --------------------------------------------------------------------------------------------------------------------

JpegApp2Segment::JpegApp2Segment(Offset_t offset, int size, Endianness e) : JpegSegment(offset, size, e)
{
}

// --------------------------------------------------------------------------------------------------------------------
//		class JpegApp2Segment -- // Usually ICC profile
// --------------------------------------------------------------------------------------------------------------------

JpegApp3Segment::JpegApp3Segment(Offset_t offset, int size, Endianness e) : JpegSegment(offset, size, e)
{
}

// --------------------------------------------------------------------------------------------------------------------
//		class JpegOtherAppSegment
// --------------------------------------------------------------------------------------------------------------------

JpegOtherAppSegment::JpegOtherAppSegment(Offset_t offset, int size, Endianness e) : JpegSegment(offset, size, e)
{
}


// --------------------------------------------------------------------------------------------------------------------
//		class JpegQuantizationTable
// --------------------------------------------------------------------------------------------------------------------

JpegQuantizationTable::JpegQuantizationTable(Offset_t offset, int size, Endianness e) : JpegSegment(offset, size, e)
{
}


// --------------------------------------------------------------------------------------------------------------------
//		class JpegStartOfFrame
// --------------------------------------------------------------------------------------------------------------------

JpegStartOfFrame::JpegStartOfFrame(Offset_t offset, int size, Endianness e) : JpegSegment(offset, size, e), m_precision(0), m_width(0), m_length(0), m_num_components(0), m_component_info{}
{
}


void JpegStartOfFrame::InterpretData()
{
	ASSERT(vibo::size(m_data) > 10);

	SetLabel(JpegMarkerString(m_data));
	m_precision = vibo::MakeUByte(&m_data[4]);
	m_length = vibo::MakeUShort(&m_data[5], FileEndianness());
	m_width = vibo::MakeUShort(&m_data[7], FileEndianness());
	m_num_components = vibo::MakeUByte(&m_data[9]);

	ASSERT(vibo::size(m_data) == 10 + 3 * m_num_components);
	for (int i = 0; i < m_num_components; ++i)
	{
		Component_info info;
		info.id = vibo::MakeUByte(&m_data[10 + 3 * i]);
		info.sampling_factors = vibo::MakeUByte(&m_data[11 + 3 * i]);
		info.quantitation_table_number = vibo::MakeUByte(&m_data[12 + 3 * i]);
		m_component_info.push_back(info);
	}
}


std::vector<std::wstring> JpegStartOfFrame::StringRepresentation() const
{
	std::vector<std::wstring> vec = FileSegment::StringRepresentation();
	std::wstringstream ss1, ss2, ss3, ss4;
	ss1 << L"         Width:        " << std::dec << m_width;
	ss2 << L"         Length:       " << std::dec << m_length;
	ss3 << L"         Precision:    " << std::dec << m_precision;
	ss4 << L"         N components: " << std::dec << m_num_components;
	vec.push_back(ss1.str());
	vec.push_back(ss2.str());
	vec.push_back(ss3.str());
	vec.push_back(ss4.str());
	for (int i = 0; i < m_num_components; ++i)
	{
		std::wstringstream ss5;
		ss5 << std::dec << L"         ID:" << m_component_info[i].id << L"  SF:" << m_component_info[i].sampling_factors << L"  QTab:" << m_component_info[i].quantitation_table_number;
		vec.push_back(ss5.str());
	}
	return vec;
}



int JpegStartOfFrame::GetPrecision() const
{
	return m_precision;
}


int JpegStartOfFrame::GetImageWidth() const
{
	return m_width;
}

int JpegStartOfFrame::GetImageLength() const
{
	return m_length;
}

int JpegStartOfFrame::GetNumComponents() const
{
	return m_num_components;
}


int JpegStartOfFrame::GetHorizontalSamplingFactor(unsigned component) const
{
	ASSERT(component >= 0 && component < m_component_info.size());
	int sf = m_component_info[component].sampling_factors;
	int retval = (sf >> 4);
	ASSERT(retval >= 1 && retval <= 4);
	return retval;
}


int JpegStartOfFrame::GetVerticalSamplingFactor(unsigned component) const
{
	ASSERT(component >= 0 && component < m_component_info.size());
	int sf = m_component_info[component].sampling_factors;
	int retval = (sf & 7);
	ASSERT(retval >= 1 && retval <= 4);
	return retval;
}


// --------------------------------------------------------------------------------------------------------------------
//		class JpegHuffmanTable
// --------------------------------------------------------------------------------------------------------------------

JpegHuffmanTable::JpegHuffmanTable(Offset_t offset, int size, Endianness e) : JpegSegment(offset, size, e)
{
}


// --------------------------------------------------------------------------------------------------------------------
//		class JpegStartOfScan
// --------------------------------------------------------------------------------------------------------------------

JpegStartOfScan::JpegStartOfScan(Offset_t offset, int size, Endianness e) : JpegSegment(offset, size, e)
{
}


// --------------------------------------------------------------------------------------------------------------------
//		class JpegImageData
// --------------------------------------------------------------------------------------------------------------------

JpegImageData::JpegImageData(Offset_t offset, int size, Endianness e) : JpegSegment(offset, size, e)
{
}


// --------------------------------------------------------------------------------------------------------------------
//		class JpegNumberOfLines
// --------------------------------------------------------------------------------------------------------------------

JpegNumberOfLines::JpegNumberOfLines(Offset_t offset, int size, Endianness e) : JpegSegment(offset, size, e)
{
}


// --------------------------------------------------------------------------------------------------------------------
//		class JpegRestartInterval
// --------------------------------------------------------------------------------------------------------------------

JpegRestartInterval::JpegRestartInterval(Offset_t offset, int size, Endianness e) : JpegSegment(offset, size, e)
{
}


// --------------------------------------------------------------------------------------------------------------------
//		class JpegSpecialSegment
// --------------------------------------------------------------------------------------------------------------------

JpegSpecialSegment::JpegSpecialSegment(Offset_t offset, int size, Endianness e) : JpegSegment(offset, size, e)
{
}


// --------------------------------------------------------------------------------------------------------------------
//		class JpegCommentSegment
// --------------------------------------------------------------------------------------------------------------------

JpegCommentSegment::JpegCommentSegment(Offset_t offset, int size, Endianness e) : JpegSegment(offset, size, e)
{
}


// --------------------------------------------------------------------------------------------------------------------
//		class JpegReservedSegment
// --------------------------------------------------------------------------------------------------------------------

JpegReservedSegment::JpegReservedSegment(Offset_t offset, int size, Endianness e) : JpegSegment(offset, size, e)
{
}


// --------------------------------------------------------------------------------------------------------------------
//		class JpegUnknownSegment
// --------------------------------------------------------------------------------------------------------------------

JpegUnknownSegment::JpegUnknownSegment(Offset_t offset, int size, Endianness e) : JpegSegment(offset, size, e)
{
}


// ****************************************************************************************************************************************************************************************
// ****************************************************************************************************************************************************************************************
// ****************************************************************************************************************************************************************************************
// ****************************************************************************************************************************************************************************************



// --------------------------------------------------------------------------------------------------------------------
//		Free function ReadJpegStartOfImage()
//
//		filepos on entry: doesn't matter
//		filepos on exit:  offset + 2
// --------------------------------------------------------------------------------------------------------------------

void ReadJpegStartOfImage(FILE* f, GraphicsVector& G, Offset_t offset, const std::wstring& comment)
{
	int check = fseek(f, offset, SEEK_SET);
	ASSERT(check == 0);

	std::shared_ptr<FileSegment> S = CreateSegment(Segmenttype::JpegStartOfImage, Endianness::Big, offset, 2);
	S->ReadData(f);
	if (!comment.empty())
	{
		S->SetLabel(comment);
	}
	ASSERT(S->GetDataByte(0) == 0xff);
	ASSERT(S->GetDataByte(1) == 0xd8);
	AddSegmentNopad(G, S);
}


// --------------------------------------------------------------------------------------------------------------------
//		Free function ReadJpegEndOfImage()
//
//		filepos on entry: doesn't matter
//		filepos on exit:  offset + 2
// --------------------------------------------------------------------------------------------------------------------

void ReadJpegEndOfImage(FILE* f, GraphicsVector& G, Offset_t offset, const std::wstring& comment)
{
	int check = fseek(f, offset, SEEK_SET);
	ASSERT(check == 0);

	std::shared_ptr<FileSegment> S = CreateSegment(Segmenttype::JpegEndOfImage, Endianness::Big, offset, 2);
	S->ReadData(f);
	if (!comment.empty())
	{
		S->SetLabel(comment);
	}
	ASSERT(S->GetDataByte(0) == 0xff);
	ASSERT(S->GetDataByte(1) == 0xd9);
	AddSegmentNopad(G, S);
}


// --------------------------------------------------------------------------------------------------------------------
//		Free function ReadJpegRestartMarker()
//
//		filepos on entry: doesn't matter
//		filepos on exit:  offset + 2
//		NOTE: I think this only appears in the data section, but include a handling function to be on the safe side.
// --------------------------------------------------------------------------------------------------------------------

void ReadJpegRestartMarker(FILE* f, GraphicsVector& G, Offset_t offset)
{
	int check = fseek(f, offset, SEEK_SET);
	ASSERT(check == 0);
	
	std::shared_ptr<FileSegment> S = CreateSegment(Segmenttype::JpegRestartMarker, Endianness::Big, offset, 2);
	S->ReadData(f);
	ASSERT(S->GetDataByte(0) == 0xff);
	int databyte2 = S->GetDataByte(1);
	ASSERT(databyte2 >= 0xd0 && databyte2 <= 0xd7);
	AddSegmentNopad(G, S);
}


void ReadJpegUnspecifiedSegment(FILE* f, GraphicsVector& G, Segmenttype seg, Offset_t offset)
{
	int check = fseek(f, offset + 2, SEEK_SET); // offset + 2: Skip ff xx signature
	ASSERT(check == 0);
	UShort_t length = vibo::GetUShort(f, Endianness::Big); // JPEG is bigendian
	length += 2; // Because the length that is stored in the segment does not include the initial 2 bytes (ff e2 etc.)

	check = fseek(f, offset, SEEK_SET); // Go back to start of segment
	ASSERT(check == 0);

	std::shared_ptr<FileSegment> S = CreateSegment(seg, Endianness::Big, offset, length);
	S->ReadData(f);
	ASSERT(S->GetDataByte(0) == 0xff);

	AddSegmentNopad(G, S);
}


// --------------------------------------------------------------------------------------------------------------------
//		Free function ReadJpegImagedata
//
//		filepos on entry: doesn't matter
//		filepos on exit:  just past the segment starting at offset, i.e. offset + length
// --------------------------------------------------------------------------------------------------------------------

void ReadJpegImagedata(FILE* f, GraphicsVector& G)
{
	int filepos = ftell(f);
	int filepos2 = filepos;
	bool eoi_marker_found = false;
	while (!feof(f))
	{
		int b1 = vibo::GetByte(f);
		if (b1 == EOF)
		{
			break;
		}
		if (b1 != 0xff) continue;
		int b2 = vibo::GetByte(f);
		if (b2 == EOF)
		{
			break;
		}
		else if (b2 == 0)
		{
			continue; // ff 00 is the normal encoding of an ff data byte
		}
		else if (b2 == 0xd9)
		{
			// ff d9 is End of image (and eof) marker!
			filepos2 = ftell(f);
			ASSERT(filepos2 != EOF);
			eoi_marker_found = true;
			break;
		}
		else if (b2 >= 0xd0 && b2 <= 0xd7)
		{
			// Ignore -- sync markers!
		}
		else
		{
			std::wcerr << L"Warning: ff " << std::hex << b2 << L" appeared in jpeg image data stream!" << std::endl;
		}
	}
	if (!eoi_marker_found)
	{
		std::cout << L"*** ERROR: Unexpected EOF ***\n";
		return;
	}
	Offset_t imagedatasize = filepos2 - filepos - 2; // -2 because we don't include the end-of-image marker

	std::shared_ptr<FileSegment> S = CreateSegment(Segmenttype::JpegImageData, Endianness::Big, filepos, imagedatasize);
	S->ReadData(f); // filepos is now at filepos + imagedatasize
	AddSegmentNopad(G, S);
	return;
}


// --------------------------------------------------------------------------------------------------------------------
//		ReadJpegFileOrEmbeddedSection()
//
//		filepos on entry: doesn't matter
//		filepos on exit:  undefined
// --------------------------------------------------------------------------------------------------------------------

void ReadJpegFileOrEmbeddedSection(FILE* f, GraphicsVector& G, Offset_t offset, int datasize, const std::wstring& comment)
{
	int chk = fseek(f, offset, SEEK_SET);
	ASSERT(chk == 0);

	int nesting = 0;

	ByteVector vec = vibo::GetBytes(f, 2);
	if (vec[0] == 0xff && vec[1] == 0xd8)
	{
		ReadJpegStartOfImage(f, G, offset, comment); // filepos is now offset+2
		unsigned char prev_marker[2];
		Offset_t endoffset = offset + datasize;
		for (;;)
		{
			// The file position is correctly positioned on entry, and should be updated correctly
			Offset_t filepos = ftell(f);
			vec = vibo::GetBytes(f, 2);
			if (filepos > endoffset)
			{
				ASSERT(false); // Done this way to allow setting a breakpoint
			}
			ASSERT(filepos > offset);
			prev_marker[0] = vec[0];
			prev_marker[1] = vec[1];
			if (vec[0] == 0xff)
			{
				if (vec[1] == 0xd9)
				{
					ReadJpegEndOfImage(f, G, filepos, comment);
					Offset_t currentpos = ftell(f);
					if (true || currentpos >= endoffset) // Return anyway -- Nikon B700 images has new StartOfImage near the end of the file!
					{
						return; // DNG may have two contiguous jpegs! (referenced by different directories)
					}
					if (nesting > 0) --nesting;
				}
				else if (vec[1] == 0xd8)
				{
					//!! THROW(L"Unexpected start of JPEG image marker (nesting not allowed)");
					int chk = fseek(f, filepos, SEEK_SET);
					ASSERT(chk == 0);
					++nesting;
					ReadJpegStartOfImage(f, G, offset, L"NESTED SEGMENT");
				}
				else if (vec[1] == 0xc4)
				{
					ReadJpegUnspecifiedSegment(f, G, Segmenttype::JpegHuffmanTable, filepos);
				}
				else if (vec[1] == 0xcc)
				{
					// Define Arithmetic conditioning table
					ReadJpegUnspecifiedSegment(f, G, Segmenttype::JpegSpecialSegment, filepos);

				}
				else if (vec[1] >= 0xc0 && vec[1] <= 0xcf)
				{
					ReadJpegUnspecifiedSegment(f, G, Segmenttype::JpegStartOfFrame, filepos);

				}
				else if (vec[1] >= 0xd0 && vec[1] <= 0xd7)
				{
					// Restart interval 'm' modulo 8 *
					// This marker has no data, requires special processing.
					void ReadJpegRestartMarker(FILE* f, GraphicsVector& G, Offset_t offset);
				}
				else if (vec[1] == 0xda)
				{
					// Start of scan
					ReadJpegUnspecifiedSegment(f, G, Segmenttype::JpegStartOfScan, filepos);
				}
				else if (vec[1] == 0xdb)
				{// Define quantization table(s)
					ReadJpegUnspecifiedSegment(f, G, Segmenttype::JpegQuantizationTable, filepos);

				}
				else if (vec[1] == 0xdc)
				{
					// Define number of lines
					ReadJpegUnspecifiedSegment(f, G, Segmenttype::JpegNumberOfLines, filepos);

				}
				else if (vec[1] == 0xdd)
				{
					// Define restart interval
					ReadJpegUnspecifiedSegment(f, G, Segmenttype::JpegRestartInterval, filepos);
				}
				else if (vec[1] == 0xde)
				{
					// Define hierarchical progression
					ReadJpegUnspecifiedSegment(f, G, Segmenttype::JpegSpecialSegment, filepos);
				}
				else if (vec[1] == 0xdf)
				{
					// Expand reference component(s)
					ReadJpegUnspecifiedSegment(f, G, Segmenttype::JpegSpecialSegment, filepos);
				}
				else if (vec[1] == 0xe0)
				{
					// jfif header
					ReadJpegUnspecifiedSegment(f, G, Segmenttype::JpegApp0Segment, filepos);
				}
				else if (vec[1] == 0xe1)
				{
					// exif header or segment
					ReadJpegUnspecifiedSegment(f, G, Segmenttype::JpegApp1Segment, filepos);
				}
				else if (vec[1] == 0xe2)
				{
					// Usually ICC definition
					ReadJpegUnspecifiedSegment(f, G, Segmenttype::JpegApp2Segment, filepos);
				}
				else if (vec[1] >= 0xe3 && vec[1] <= 0xef)
				{
					// Other APP marker
					ReadJpegUnspecifiedSegment(f, G, Segmenttype::JpegOtherAppSegment, filepos);
				}
				else if (vec[1] == 0xfe)
				{
					// Label
					ReadJpegUnspecifiedSegment(f, G, Segmenttype::JpegCommentSegment, filepos);
				}
				else if (vec[1] == 0x01)
				{
					// For temporary private use in arithmetic coding *
					ReadJpegUnspecifiedSegment(f, G, Segmenttype::JpegSpecialSegment, filepos);
				}
				else if ((vec[1] > 0x02 && vec[1] <= 0xbf) || (vec[1] >= 0xf0 && vec[1] <= 0xfd) || vec[1] == 0xc8)
				{
					// Reserved
					ReadJpegUnspecifiedSegment(f, G, Segmenttype::JpegReservedSegment, filepos);
				}
				else
				{
					ReadJpegUnspecifiedSegment(f, G, Segmenttype::JpegUnknownSegment, filepos);
				}
			}
			else
			{
				break;
			}
			if (prev_marker[0] == 0xff && prev_marker[1] == 0xda)
			{
				// Start of scan marker was the last one read, i.e. image data should follow
				ReadJpegImagedata(f, G);
			}
		}
	}
	else
	{
		THROW(L"JPEG data was expected!");
	}
}


std::wstring JpegMarkerString(const ByteVector& vec)
{
	std::wstringstream s;
	if (vibo::size(vec) >= 2)
	{
		s << L"Marker:" << std::hex << int{ vec[0] } << L" " << int{ vec[1] };
	}
	else
	{
		s << L"*** ERRROR: Empty vector ***";
	}
	return s.str();
}

