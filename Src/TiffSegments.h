// File: TiffSegments.h
// This file is part of the project: Rewrap-jpeg-as-tiff -- Rewraps a jpeg file into a TIFF container, without loss of information (no re-encoding).
// Copyright(c) 2018 Vidar Bosnes
// This software is licenced under the GNU GENERAL PUBLIC LICENSE Version 3

#ifndef TIFFSEGMENTS_H_INCLUDED
#define TIFFSEGMENTS_H_INCLUDED

#include "FileSegment.h"
#include "GraphicsFile.h"
#include "Exception.h"
#include "TiffDirEntry.h"
#include <vector>

// --------------------------------------------------------------------------------------------------------------------
//		Free functions
// --------------------------------------------------------------------------------------------------------------------

Offset_t ReadTiffHeader(FILE* f, Filetype ft, GraphicsVector& G, Offset_t offset); // returns offset of first directory
void ReadTiffDirectories(FILE* f, Filetype ft, GraphicsVector& G, Offset_t offset);
void ReadTiffOtherData(FILE* f, GraphicsVector& G, Segmenttype seg, Endianness e, int offset, int datasize);

std::vector<uint32_t> ReadTiffNumericVector(FILE* f, Endianness e, const TiffDirEntry& E);


// --------------------------------------------------------------------------------------------------------------------
//		Derived TIFF Segment
// --------------------------------------------------------------------------------------------------------------------

class TiffSegment : public FileSegment
{
protected:
	Endianness m_Endianness;

public:
	TiffSegment(int offset, int size, Endianness e);
	Endianness FileEndianness() const;
protected:
	~TiffSegment() = default;
};


class TiffHeader : public TiffSegment
{
	int m_directoryOffset;

public:
	TiffHeader(int offset, int size, Endianness e);
	~TiffHeader() = default;

	Offset_t GetDirectoryOffset() const;
	void SetDirectoryOffset(int offset);
	void RebuildBinaryData();
	std::vector<std::wstring> StringRepresentation() const override;

protected:
	void InterpretData() override;
};


class TiffDirectory : public TiffSegment
{
	std::vector<TiffDirEntry> m_entries;
	int m_nextDirectoryOffset;

public:
	TiffDirectory(int offset, int size, Endianness e);
	~TiffDirectory() = default;

	void AddEntry(const TiffDirEntry& E);
	std::vector<std::wstring> StringRepresentation() const override;

	Offset_t GetNextDirectoryOffset() const;
	void SetNextDirectoryOffset(int offset);
	int GetCompression();
	void ReadExternalData(FILE* f, GraphicsVector& G);
	void RebuildBinaryData() override;
	void SortEntries(); // "According to the standard, tags must appear in numerical order"

protected:
	void InterpretData() override;
};


template<typename T, int tiffDatatype> class TiffNumericVectorT : public TiffSegment
{
protected:	
	int m_datatype;
	int m_datacount;
	std::vector<T> m_vector;

public:
	TiffNumericVectorT(int offset, int size, Endianness e);
	int GetTiffDatatype() const;
	int GetTiffDatacount() const;
	~TiffNumericVectorT() = default;
	void push_back(const T&);
	void assign(const std::vector<T>&);
	void RebuildBinaryData() override;

protected:
	void InterpretData() override;
};

template<typename T, int tiffDatatype> TiffNumericVectorT<T, tiffDatatype>::TiffNumericVectorT(int offset, int size, Endianness e) : TiffSegment(offset, size, e), m_datatype(tiffDatatype), m_datacount(0), m_vector{}
{
	ASSERT(TiffDatatypeLength(m_datatype) >= 1);
	this->m_datacount = m_size / TiffDatatypeLength(m_datatype);
	ASSERT(m_datacount * TiffDatatypeLength(m_datatype) == m_size);
}

template<typename T, int tiffDatatype> int TiffNumericVectorT<T, tiffDatatype>::GetTiffDatatype() const
{
	return tiffDatatype;
}

template<typename T, int tiffDatatype> int TiffNumericVectorT<T, tiffDatatype>::GetTiffDatacount() const
{
	return m_datacount;
}


template<typename T, int tiffDatatype> void TiffNumericVectorT<T, tiffDatatype>::InterpretData()
{
	int sizeof_tiffDatatype = TiffDatatypeLength(m_datatype);
	ASSERT(sizeof(T) >= sizeof_tiffDatatype);
	int sizeof_data = m_datacount * sizeof_tiffDatatype;
	ASSERT(m_size == sizeof_data);
	ASSERT(vibo::size(m_data) == sizeof_data);

	m_vector.resize(m_datacount);

	if (sizeof_tiffDatatype == 1)
	{
		for (int i = 0; i < m_datacount; ++i)
		{
			m_vector[i] = vibo::MakeUByte(&m_data[i]);
		}
	}
	else if (sizeof_tiffDatatype == 2)
	{
		for (int i = 0; i < m_datacount; ++i)
		{
			m_vector[i] = (T) vibo::MakeUShort(&m_data[sizeof_tiffDatatype*i], FileEndianness());
		}
	}
	else if (sizeof_tiffDatatype == 4)
	{
		for (int i = 0; i < m_datacount; ++i)
		{
			m_vector[i] = (T) vibo::MakeULong(&m_data[sizeof_tiffDatatype*i], FileEndianness());
		}
	}
	else
	{
		THROW(L"TiffNumericVectorT::InterpretData: The datatype must be either 1, 2 or 4 bytes long");
	}
}

template<typename T, int tiffDatatype> void TiffNumericVectorT<T, tiffDatatype>::RebuildBinaryData()
{
	int sizeof_tiffDatatype = TiffDatatypeLength(m_datatype);
	ASSERT(sizeof_tiffDatatype > 0);
	m_datacount = vibo::size(m_vector);
	ASSERT(m_datacount > 0);
	m_size = m_datacount * sizeof_tiffDatatype;
	m_data.resize(m_size);

	vibo::binary_copy(&m_data[0], reinterpret_cast<unsigned char*>(&m_vector[0]), m_datacount, sizeof_tiffDatatype, FileEndianness());
}



template<typename T, int tiffDatatype> void TiffNumericVectorT<T, tiffDatatype>::push_back(const T& num)
{
	m_vector.push_back(num);
	m_datacount = vibo::size(m_vector);
	RebuildBinaryData();
}


template<typename T, int tiffDatatype> void TiffNumericVectorT<T, tiffDatatype>::assign(const std::vector<T>& rhs)
{
	m_vector = rhs;
	m_datacount = vibo::size(m_vector);
	RebuildBinaryData();
}


class TiffByteVector : public TiffNumericVectorT <unsigned char, Datatype::Ubyte>
{
public:
	TiffByteVector(int offset, int size, Endianness e);
	~TiffByteVector() = default;
	void RebuildBinaryData() override;
};


class TiffUShortVector : public TiffNumericVectorT <uint16_t, Datatype::Ushort>
{
public:
	TiffUShortVector(int offset, int size, Endianness e);
	~TiffUShortVector() = default;
};



class TiffOffsetTable : public TiffNumericVectorT<uint32_t, Datatype::Ulong>
{
public:
	TiffOffsetTable(int offset, int size, Endianness e);
	~TiffOffsetTable() = default;
};


class TiffBytecountTable : public TiffNumericVectorT<uint32_t, Datatype::Ulong>
{
public:
	TiffBytecountTable(int offset, int size, Endianness e);
	~TiffBytecountTable() = default;
};


class TiffImageData : public TiffSegment
{
public:
	TiffImageData(int offset, int size, Endianness e);
	~TiffImageData() = default;
};


#endif

