// File: TiffSegments.cpp
// This file is part of the project: Rewrap-jpeg-as-tiff -- Rewraps a jpeg file into a TIFF container, without loss of information (no re-encoding).
// Copyright(c) 2018 Vidar Bosnes
// This software is licenced under the GNU GENERAL PUBLIC LICENSE Version 3

#include "TiffSegments.h"
#include <iostream>
#include <iomanip>
#include "Exception.h"
#include "JpegSegments.h"
#include "Util.h"
#include <algorithm> // std::sort
#include "CreateSegment.h"


std::shared_ptr<FileSegment> ReadTiffSegmentGeneric(FILE* f, Segmenttype seg, Endianness e, int offset, int datasize);

// --------------------------------------------------------------------------------------------------------------------
//		class TiffSegment
// --------------------------------------------------------------------------------------------------------------------

TiffSegment::TiffSegment(int offset, int size, Endianness e) : FileSegment(offset, size), m_Endianness(e)
{
}


Endianness TiffSegment::FileEndianness() const
{
	return m_Endianness;
}


// --------------------------------------------------------------------------------------------------------------------
//		class TiffHeader
// --------------------------------------------------------------------------------------------------------------------

TiffHeader::TiffHeader(int offset, int size, Endianness e) : TiffSegment(offset, size, e), m_directoryOffset(0)
{
	if (e == Endianness::Little)
	{
		SetLabel(L"Little-endian");
	}
	else if (e == Endianness::Big)
	{
		SetLabel(L"Big-endian");
	}
	else
	{
		ASSERT(false);
	};
}


void TiffHeader::SetDirectoryOffset(int offset)
{
	m_directoryOffset = offset;
}


Offset_t TiffHeader::GetDirectoryOffset() const
{
	return m_directoryOffset;
}


void TiffHeader::RebuildBinaryData()
{
	int32_t dirOffset = m_directoryOffset;
	unsigned char* bytes = reinterpret_cast<unsigned char*>(&dirOffset);

	if (m_Endianness == Endianness::Little)
	{
		m_data = ByteVector{ 0x49, 0x49, 0x2a, 0x00 };
	}
	else if (m_Endianness == Endianness::Big)
	{
		m_data = ByteVector{ 0x4d, 0x4d, 0x00, 0x2a };
	}
	else
	{
		ASSERT(false);
	}
	if (m_Endianness == vibo::GetSystemEndianness())
	{
		// Keep endianness
		m_data.push_back(bytes[0]);
		m_data.push_back(bytes[1]);
		m_data.push_back(bytes[2]);
		m_data.push_back(bytes[3]);
	}
	else
	{
		// Switch endianness
		m_data.push_back(bytes[3]);
		m_data.push_back(bytes[2]);
		m_data.push_back(bytes[1]);
		m_data.push_back(bytes[0]);
	}
}


void TiffHeader::InterpretData()
{
	ASSERT(vibo::size(m_data) == 8);
	m_directoryOffset = vibo::MakeULong(&m_data[4], FileEndianness());
}


std::vector<std::wstring> TiffHeader::StringRepresentation() const
{
	auto vec = FileSegment::StringRepresentation();
	std::wstring str = L"Directory offset: " + std::to_wstring(m_directoryOffset);
	vec.push_back(str);
	return vec;
}


// --------------------------------------------------------------------------------------------------------------------
//		class TiffDirectory
// --------------------------------------------------------------------------------------------------------------------

TiffDirectory::TiffDirectory(int offset, int size, Endianness e) : TiffSegment(offset, size, e), m_entries(), m_nextDirectoryOffset(0)
{
}


void TiffDirectory::AddEntry(const TiffDirEntry& E)
{
	m_entries.push_back(E);
}


std::vector<std::wstring> TiffDirectory::StringRepresentation() const
{
	auto vec = FileSegment::StringRepresentation();
	for (auto p = m_entries.begin(); p != m_entries.end(); ++p)
	{
		vec.push_back(p->StringRepresentation(FileEndianness()));
	}
	std::wstring str = L"Next directory:" + std::to_wstring(m_nextDirectoryOffset);
	vec.push_back(str);
	return vec;
}


int TiffDirectory::GetCompression()
{
	for (auto p = m_entries.begin(); p != m_entries.end(); ++p)
	{
		if (p->Tag() == TiffTag::Compression)
		{
			return p->GetIntegerValue();
		}
	}
	ASSERT(false);
	return 0; // Should not be reached
}


void TiffDirectory::InterpretData()
{
	int num_entries = vibo::MakeUShort(&m_data[0], FileEndianness());
	ASSERT(vibo::size(m_data) == 12 * num_entries + 6);

	for (int i = 0; i < num_entries; ++i)
	{
		TiffDirEntry e;
		e.InitializeFromMemory(&m_data[2 + 12 * i], FileEndianness());
		AddEntry(e);
	}
	m_nextDirectoryOffset = vibo::MakeULong(&m_data[2 + 12 * num_entries], FileEndianness());
}


void TiffDirectory::RebuildBinaryData()
{
	int num_entries = vibo::size(m_entries);
	m_size = 6 + 12 * num_entries;
	m_data.resize(m_size);
	unsigned char* mem = &m_data[0];
	vibo::PutUShort(mem, num_entries, FileEndianness());
	for (int i = 0; i < num_entries; ++i)
	{
		m_entries[i].BuildMemoryRepresentation(mem + 2 + 12 * i, FileEndianness());
	}
	vibo::PutUlong(mem + 2 + 12 * num_entries, m_nextDirectoryOffset, FileEndianness());
}


// --------------------------------------------------------------------------------------------------------------------
//		TiffDirectory::ReadExternalData()
//
//		filepos on entry: doesn't matter
//		filepos on exit:  same as on entry
//
// --------------------------------------------------------------------------------------------------------------------

void TiffDirectory::ReadExternalData(FILE* f, GraphicsVector& G)
{
	Offset_t filepos = ftell(f);
	std::vector<uint32_t> stripOffsets;
	std::vector<uint32_t> stripByteCounts;
	std::vector<uint32_t> tileOffsets;
	std::vector<uint32_t> tileByteCounts;
	std::vector<uint32_t> bitsPerSample;

	int compression = 0;
	for (auto p = m_entries.begin(); p != m_entries.end(); ++p)
	{
		switch (p->Tag())
		{

		case TiffTag::BitsPerSample:
			bitsPerSample = ReadTiffNumericVector(f, FileEndianness(), *p);
			if (p->GetDataSize() > 4)
			{
				std::shared_ptr<FileSegment> S = ReadTiffSegmentGeneric(f, Segmenttype::TiffUShortVector, FileEndianness(), p->GetOffsetField(), p->GetDataSize());
				Offset_t filepos = ftell(f);
				S->ReadData(f);
				int chk = fseek(f, filepos, SEEK_SET);
				ASSERT(chk == 0);
				S->SetLabel(TiffTagName(TiffTag::BitsPerSample));

				AddSegmentNopad(G, S);
			}
			break;

		case TiffTag::StripOffsets:
			ASSERT(vibo::size(tileOffsets) == 0); // Image cannot be both stip-based and tile-based!
			ASSERT(vibo::size(stripOffsets) == 0); // Throw if TiffTag::StripOffsets appears twice!
			stripOffsets = ReadTiffNumericVector(f, FileEndianness(), *p);
			if (p->GetDataSize() > 4)
			{
				ReadTiffOtherData(f, G, Segmenttype::TiffOffsetTable, FileEndianness(), p->GetOffsetField(), p->GetDataSize());
			}
			break;
		case TiffTag::StripByteCounts:
			ASSERT(vibo::size(tileByteCounts) == 0); // Image cannot be both stip-based and tile-based!
			ASSERT(vibo::size(stripByteCounts) == 0); // Throw if TiffTag::StripByteCounts appears twice!
			stripByteCounts = ReadTiffNumericVector(f, FileEndianness(), *p);
			if (p->GetDataSize() > 4)
			{
				ReadTiffOtherData(f, G, Segmenttype::TiffBytecountTable, FileEndianness(), p->GetOffsetField(), p->GetDataSize());
			}
			break;

		case TiffTag::TileOffsets:
			ASSERT(vibo::size(stripOffsets) == 0); // Image cannot be both stip-based and tile-based!
			ASSERT(vibo::size(tileOffsets) == 0); // Throw if TiffTag::TileOffsets appears twice!
			stripOffsets = ReadTiffNumericVector(f, FileEndianness(), *p);
			if (p->GetDataSize() > 4)
			{
				ReadTiffOtherData(f, G, Segmenttype::TiffOffsetTable, FileEndianness(), p->GetOffsetField(), p->GetDataSize());
			}
			break;

		case TiffTag::TileByteCounts:
			ASSERT(vibo::size(stripByteCounts) == 0); // Image cannot be both stip-based and tile-based!
			ASSERT(vibo::size(tileByteCounts) == 0); // Throw if TiffTag::StripByteCounts appears twice!
			tileByteCounts = ReadTiffNumericVector(f, FileEndianness(), *p);
			if (p->GetDataSize() > 4)
			{
				ReadTiffOtherData(f, G, Segmenttype::TiffBytecountTable, FileEndianness(), p->GetOffsetField(), p->GetDataSize());
			}
			break;

		case TiffTag::Compression:
			ASSERT(compression == 0); // Throw if TiffTag::Compression appears twice!
			compression = p->GetIntegerValue();
			break;

		case TiffTag::JPEGTables:
			ReadJpegFileOrEmbeddedSection(f, G, p->GetOffsetField(), p->GetDataSize(), L"JPEG tables in TIFF file");
			break;
		}
	}
	if (vibo::size(tileOffsets) == 1 && vibo::size(tileByteCounts) == 1 && (compression == 7 || compression == 6))
	{
		ReadJpegFileOrEmbeddedSection(f, G, tileOffsets[0], tileByteCounts[0], L"JPEG imagedata in TIFF file");
	}
	else if (vibo::size(stripOffsets) == 1 && vibo::size(stripByteCounts) == 1 && (compression == 7 || compression == 6))
	{
		ReadJpegFileOrEmbeddedSection(f, G, stripOffsets[0], stripByteCounts[0], L"JPEG imagedata in TIFF file");
	}
	else if (vibo::size(tileOffsets) >= 1 && vibo::size(tileByteCounts) >= 1)
	{
		int siz = vibo::size(tileOffsets);
		ASSERT(siz == vibo::size(tileByteCounts));
		for (int i = 0; i < siz; ++i)
		{
			ReadTiffOtherData(f, G, Segmenttype::TiffImageData, FileEndianness(), tileOffsets[i], tileByteCounts[i]);
		}
	}
	else if (vibo::size(stripOffsets) >= 1 && vibo::size(stripByteCounts) >= 1)
	{
		int siz = vibo::size(stripOffsets);
		ASSERT(siz == vibo::size(stripByteCounts));
		for (int i = 0; i < siz; ++i)
		{
			ReadTiffOtherData(f, G, Segmenttype::TiffImageData, FileEndianness(), stripOffsets[i], stripByteCounts[i]);
		}
	}
}


void TiffDirectory::SetNextDirectoryOffset(int offset)
{
	m_nextDirectoryOffset = offset;
}


Offset_t TiffDirectory::GetNextDirectoryOffset() const
{
	return m_nextDirectoryOffset;
}


bool TiffDirectory_sortfunc(const TiffDirEntry& e1, const TiffDirEntry& e2)
{
	return e1.Tag() < e2.Tag();
}


void TiffDirectory::SortEntries()
{
	std::sort(m_entries.begin(), m_entries.end(), TiffDirectory_sortfunc);
}


// --------------------------------------------------------------------------------------------------------------------
//		Class TiffByteVector
// --------------------------------------------------------------------------------------------------------------------

TiffByteVector::TiffByteVector(int offset, int size, Endianness e) : TiffNumericVectorT<unsigned char, Datatype::Ubyte>(offset, size, e)
{
}


void TiffByteVector::RebuildBinaryData()
{
	int sizeof_tiffDatatype = TiffDatatypeLength(m_datatype);
	ASSERT(sizeof_tiffDatatype > 0);
	m_datacount = vibo::size(m_vector);
	ASSERT(m_datacount > 0);
	m_size = m_datacount * sizeof_tiffDatatype;
	m_data.resize(m_size);

	vibo::binary_copy(&m_data[0], reinterpret_cast<unsigned char*>(&m_vector[0]), m_datacount, sizeof_tiffDatatype, FileEndianness());
}


// --------------------------------------------------------------------------------------------------------------------
//		Class TiffUShortVector
// --------------------------------------------------------------------------------------------------------------------

TiffUShortVector::TiffUShortVector(int offset, int size, Endianness e) : TiffNumericVectorT<uint16_t, Datatype::Ushort>(offset, size, e)
{
}


// --------------------------------------------------------------------------------------------------------------------
//		Class TiffOffsetTable
// --------------------------------------------------------------------------------------------------------------------

TiffOffsetTable::TiffOffsetTable(int offset, int size, Endianness e) : TiffNumericVectorT<uint32_t, Datatype::Ulong>(offset, size, e)
{
}


// --------------------------------------------------------------------------------------------------------------------
//		Class TiffBytecountTable
// --------------------------------------------------------------------------------------------------------------------

TiffBytecountTable::TiffBytecountTable(int offset, int size, Endianness e) : TiffNumericVectorT<uint32_t, Datatype::Ulong>(offset, size, e)
{
}


// --------------------------------------------------------------------------------------------------------------------
//		Class TiffImageData
// --------------------------------------------------------------------------------------------------------------------

TiffImageData::TiffImageData(int offset, int size, Endianness e) : TiffSegment(offset, size, e)
{
}


// --------------------------------------------------------------------------------------------------------------------
//		Free function: ReadTiffHeader()
//
//		filepos on entry: doesn't matter, uses offset argument
//		filepos on exit:  offset + sizeof(header) // sizeof(header) == 8
// --------------------------------------------------------------------------------------------------------------------

Offset_t ReadTiffHeader(FILE* f, Filetype ft, GraphicsVector& G, Offset_t offset)
{
	int chk = fseek(f, offset, SEEK_SET);
	ASSERT(chk == 0);

	std::shared_ptr<FileSegment> S = CreateSegment(Segmenttype::TiffHeader, GetEndianness(ft), 0, 8);
	std::shared_ptr<TiffHeader> P = std::dynamic_pointer_cast<TiffHeader>(S);
	ASSERT(P != nullptr);
	P->ReadData(f);
	Offset_t first_directory_offset = P->GetDirectoryOffset();
	AddSegmentNopad(G, S);
	return first_directory_offset;
}


// --------------------------------------------------------------------------------------------------------------------
//		Free function: ReadTiffDirectories()
//
//		filepos on entry: doesn't matter, uses offset argument
//		filepos on exit:  just past the end of the last directory in the linked list of directories
// --------------------------------------------------------------------------------------------------------------------

void ReadTiffDirectories(FILE* f, Filetype ft, GraphicsVector& G, Offset_t offset)
{
	Offset_t filepos = offset;

	while (filepos > 0)
	{
		int check = fseek(f, filepos, SEEK_SET);
		ASSERT(check == 0);

		int num_entries = vibo::GetUShort(f, GetEndianness(ft));
		int siz = 12 * num_entries + 6; // 2: num entries, 4: next

		std::shared_ptr<FileSegment> S = CreateSegment(Segmenttype::TiffDirectory, GetEndianness(ft), filepos, siz);
		std::shared_ptr<TiffDirectory> P = std::dynamic_pointer_cast<TiffDirectory>(S);
		ASSERT(P != nullptr);

		// Read data as binary chunk
		check = fseek(f, filepos, SEEK_SET);
		ASSERT(check == 0);
		P->ReadData(f);
		filepos = P->GetNextDirectoryOffset();

		AddSegmentNopad(G, S);
		P->ReadExternalData(f, G);
	}
}

// --------------------------------------------------------------------------------------------------------------------
//		Free function: ReadTiffOtherData()
//
//		filepos on entry: doesn't matter
//		filepos on exit:  same as on entry
// --------------------------------------------------------------------------------------------------------------------

void ReadTiffOtherData(FILE* f, GraphicsVector& G, Segmenttype seg, Endianness e, int offset, int datasize)
{
	std::shared_ptr<FileSegment> S = ReadTiffSegmentGeneric(f, seg, e, offset, datasize);
	AddSegmentNopad(G, S); 
}


// --------------------------------------------------------------------------------------------------------------------
//		Free function: ReadTiffSegmentGeneric()
//
//		filepos on entry: doesn't matter
//		filepos on exit:  same as on entry
// --------------------------------------------------------------------------------------------------------------------

std::shared_ptr<FileSegment> ReadTiffSegmentGeneric(FILE* f, Segmenttype seg, Endianness e, int offset, int datasize)
{
	int filepos_bk = ftell(f);

	int chk = fseek(f, offset, SEEK_SET);
	ASSERT(chk == 0);

	std::shared_ptr<FileSegment> S = CreateSegment(seg, e, offset, datasize);
	S->ReadData(f);

	chk = fseek(f, filepos_bk, SEEK_SET);
	ASSERT(chk == 0);
	return S;
}


// --------------------------------------------------------------------------------------------------------------------
//		Free function: ReadTiffNumericVector()
//
//		filepos on entry: doesn't matter
//		filepos on exit:  same as on entry
// --------------------------------------------------------------------------------------------------------------------

std::vector<uint32_t> ReadTiffNumericVector(FILE* f, Endianness e, const TiffDirEntry& E)
{
	Offset_t filepos_bk = ftell(f);
	std::vector<uint32_t> vec;
	int sizeof_datatype = TiffDatatypeLength(E.GetDataType());
	int datacount = E.GetDataCount();
	if (datacount == 1)
	{
		switch (sizeof_datatype)
		{
		case 1:
		{
			AsByte fb = E.GetFourBytes();
			vec.push_back(fb[0]);
			break;
		}

		case 2:
		{
			AsShort ts = E.GetTwoShorts();
			vec.push_back(ts[0]);
			break;
		}

		case 4:
		{
			uint32_t longval = E.GetLongValue();
			vec.push_back(longval);
			break;
		}

		default:
			ASSERT(false);
		}
	}
	else if (datacount == 2 && sizeof_datatype == 2)
	{
		AsShort ts = E.GetTwoShorts();
		vec.push_back(ts[0]);
		vec.push_back(ts[1]);
	}
	else if (sizeof_datatype == 1 && datacount <= 4)
	{
		AsByte fb = E.GetFourBytes();
		for (int i = 0; i < datacount; ++i)
		{
			vec.push_back(fb[i]);
		}
	}
	else if (sizeof_datatype*datacount > 4)
	{
		vec.resize(datacount);
		int check = fseek(f, E.GetOffsetField(), SEEK_SET);
		ASSERT(check == 0);
		if (sizeof_datatype == 1)
		{
			for (uint32_t& element : vec)
			{
				element = vibo::GetByte(f);
			}
		}
		else if (sizeof_datatype == 2)
		{
			for (uint32_t& element : vec)
			{
				element = vibo::GetUShort(f, e);
			}
		}
		else if (sizeof_datatype == 4)
		{
			for (uint32_t& element : vec)
			{
				element = vibo::GetULong(f, e);
			}
		}
		else
		{
			THROW(L"The datatype must be 1,2 or 4 bytes long!");
		}
	}
	else
	{
		THROW(L"This should not happen! BUG?");
	}
	int check = fseek(f, filepos_bk, SEEK_SET);
	return vec;
}

