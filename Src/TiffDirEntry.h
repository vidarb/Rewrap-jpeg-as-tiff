// File: TiffDirEntry.h
// This file is part of the project: Rewrap-jpeg-as-tiff -- Rewraps a jpeg file into a TIFF container, without loss of information (no re-encoding).
// Copyright(c) 2018 Vidar Bosnes
// This software is licenced under the GNU GENERAL PUBLIC LICENSE Version 3

#ifndef TIFFDIRENTRY_H_INCLUDED
#define TIFFDIRENTRY_H_INCLUDED

#include <stdint.h>
#include <stdio.h>
#include <string>
#include <vector>
#include "Util.h"

//	=================================================================================================================================================
//
//		A TIFF directory entry consists of a two-byte TagID, a two-byte DataType, a two-byte DataCount, and a four-byte field which can hold either
//		an offset to the data storage location, or the data itself if it is four bytes or less. Manipulating this Offset_or_Value field is tricky,
//		because both the Endianness of the Tiff File and the size of the Datatype must be taken into account when doing Enianness-switching.
//		The following logic is used:
//		TagID, DataType and DataCount are converted from the Endianness of the TIFF-file to the Endianness of the computer, and stored as ints.
//		The Offset_or_Value-field is stored as in the TIFF-file, i.e. no Endianness-switching. Endianness-switching is performed when using the data.
//		Thus, if the DataType occupies one byte, the representation is identical in a BigEndian and a LittleEndian machine.
//		If the DataType occupes two bytes and the DataCount is 1, the first and second byte may need to be switched.
//		If the DataType occupes two bytes and the DataCount is 2, the first and second, and the third and fourth byte, may need to be switched.
//		If the Datatype occupies four bytes, the first and fourth, and the second and third byte may need to be switched.
//	_________________________________________________________________________________________________________________________________________________
//
//		To enforce consistency, helper classes representing the various StorageLogics have been written. Separate constructors have been written
//		for each of the StorageLogics. Reading a value in a different storage mode than the one used in the constructor in not permitted.
//		The same StorageLogic is used for one and two short values stored locally. Different StorageLogics are used for Longs and Offsets.
//	=================================================================================================================================================

enum class StorageLogic
{
	ByteData, ShortData, LongData, OffsetData, Invalid
};


class AsByte
{
	unsigned char m_data[4];

public:
	AsByte()
	{
		m_data[0] = 0;
		m_data[1] = 0;
		m_data[2] = 0;
		m_data[3] = 0;
	}

	unsigned char operator[](int idx) const
	{
		return m_data[idx];
	}

	unsigned char& operator[](int idx)
	{
		return m_data[idx];
	}
	AsByte(const AsByte&) = default;
	AsByte& operator=(const AsByte&) = default;
	~AsByte() = default;
};

class AsShort
{
	uint16_t m_data[2];
public:
	AsShort()
	{
		m_data[0] = 0;
		m_data[1] = 0;
	}

	AsShort(int v1, int v2)
	{
		m_data[0] = v1;
		m_data[1] = v2;
	}

	AsShort(int v)
	{
		m_data[0] = v;
		m_data[1] = 0;
	}

	uint16_t operator[](int idx) const
	{
		return m_data[idx];
	}

	uint16_t& operator[](int idx)
	{
		return m_data[idx];
	}
	AsShort(const AsShort&) = default;
	AsShort& operator=(const AsShort&) = default;
	~AsShort() = default;
};


class AsOffset
{
	uint32_t m_value;
public:
	AsOffset(Offset_t v)
	{
		m_value = v;
	}
	uint32_t value() const
	{
		return m_value;
	}
	AsOffset(const AsOffset&) = default;
	AsOffset& operator=(const AsOffset&) = default;
	~AsOffset() = default;
};


//	_________________________________________________________________________________________________________________________________________________
//
//		class TiffDirEntry
//	_________________________________________________________________________________________________________________________________________________

class TiffDirEntry
{
	Endianness m_endianness;
	int m_tagID; //Short_t
	int m_dataType; // Short_t
	ULong_t m_dataCount; // ULong_t
	unsigned char m_dataBytes[4]; // Used when interpreting the value (e.g. a vector of 2 shorts where m_Offset_or_Data holds the value)
	StorageLogic m_storageLogic; // AsByte, AsShort, AsLong, AsOffset, Invalid

public:
	TiffDirEntry();
	TiffDirEntry(int tagid, int datatype, int datacount, const uint32_t& longvalue, Endianness e);
	TiffDirEntry(int tagid, int datatype, int datacount, const AsOffset& offset, Endianness e);
	TiffDirEntry(int tagid, int datatype, int datacount, const AsShort& ts, Endianness e);
	TiffDirEntry(int tagid, int datatype, int datacount, const AsByte& fb, Endianness e);
	~TiffDirEntry();
	TiffDirEntry(const TiffDirEntry&);
	TiffDirEntry& operator=(const TiffDirEntry&);
	void InitializeFromMemory(const unsigned char* mem, Endianness e);
	void BuildMemoryRepresentation(unsigned char* mem, Endianness e);
	std::wstring StringRepresentation(Endianness e) const;

	int Tag() const;
	ULong_t GetDataSize() const;
	int GetDataType() const;
	int GetDataCount() const;
	int GetElementSize() const;

	// __________________ Access methods _____________________
	
	uint32_t GetOffsetField() const;
	uint32_t GetLongValue() const;
	AsShort GetTwoShorts() const;
	AsByte GetFourBytes() const;
	int GetIntegerValue() const;
};


//	_________________________________________________________________________________________________________________________________________________
//
//		The TIFF datatypes
//	_________________________________________________________________________________________________________________________________________________

enum Datatype
{
	Ubyte = 1,
	Ascii = 2,
	Ushort = 3,
	Ulong = 4,
	Rational = 5,
	Sbyte = 6,
	Xbyte = 7,
	Sshort = 8,
	Slong = 9,
	SRational = 10,
	Float = 11,
	Double = 12,
	IFD = 13
};

//	_________________________________________________________________________________________________________________________________________________
//
//		TIFF tags -- the enum is populated with the help of a preprocessor macro.
//	_________________________________________________________________________________________________________________________________________________

enum TiffTag
{
#define TIFFTAG_MACRO(name, dir, type, value)  name=value,
#include "TiffTags.hxx"
#undef  TIFFTAG_MACRO
};

//	_________________________________________________________________________________________________________________________________________________
//
//		Free functions
//	_________________________________________________________________________________________________________________________________________________

UShort_t TiffDatatypeLength(int typ);
std::wstring TiffDataTypeString(int typ);
std::wstring TiffTagName(int ID);



#endif


