// File: TiffDirEntry.cpp
// This file is part of the project: Rewrap-jpeg-as-tiff -- Rewraps a jpeg file into a TIFF container, without loss of information (no re-encoding).
// Copyright(c) 2018 Vidar Bosnes
// This software is licenced under the GNU GENERAL PUBLIC LICENSE Version 3

#include "TiffDirEntry.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include "Exception.h"
#include "Util.h"


// Local functions

std::wstring GetSingleValueRepresentation(const unsigned char* data, int dataType, Endianness e);
std::wstring GetValuePairRepresentation(const unsigned char* data, int dataType, Endianness e);
std::wstring GetValueRepresentation(const unsigned char* dataBytes, int dataType, int datacount, Endianness e);
std::wstring GetDatatypeRepresentation(int datatype, int datacount, Endianness e);
std::wstring GetByteDataRepresentation(const unsigned char* data, int dataType, int dataCount, Endianness e);


TiffDirEntry::TiffDirEntry() : m_tagID(0), m_dataType(0), m_dataCount(0), m_storageLogic(StorageLogic::Invalid)
{
	m_endianness = Endianness::Little; // Whatever (will be overwritten)
	m_dataBytes[0] = 0;
	m_dataBytes[1] = 0;
	m_dataBytes[2] = 0;
	m_dataBytes[3] = 0;
}

TiffDirEntry::~TiffDirEntry()
{
}


TiffDirEntry::TiffDirEntry(const TiffDirEntry& rhs)
	: m_endianness(rhs.m_endianness),
	m_tagID(rhs.m_tagID),
	m_dataType(rhs.m_dataType),
	m_dataCount(rhs.m_dataCount),
	m_storageLogic(rhs.m_storageLogic)
{
	m_dataBytes[0] = rhs.m_dataBytes[0];
	m_dataBytes[1] = rhs.m_dataBytes[1];
	m_dataBytes[2] = rhs.m_dataBytes[2];
	m_dataBytes[3] = rhs.m_dataBytes[3];
}


TiffDirEntry& TiffDirEntry::operator=(const TiffDirEntry& rhs)
{
	m_endianness = rhs.m_endianness;
	m_tagID = rhs.m_tagID;
	m_dataType = rhs.m_dataType;
	m_dataCount = rhs.m_dataCount;
	m_dataBytes[0] = rhs.m_dataBytes[0];
	m_dataBytes[1] = rhs.m_dataBytes[1];
	m_dataBytes[2] = rhs.m_dataBytes[2];
	m_dataBytes[3] = rhs.m_dataBytes[3];
	m_storageLogic = rhs.m_storageLogic;
	return *this;
}


TiffDirEntry::TiffDirEntry(int tagid, int datatype, int datacount, const AsOffset& offset, Endianness e) : m_endianness(e), m_tagID(tagid), m_dataType(datatype), m_dataCount(datacount), m_storageLogic(StorageLogic::OffsetData)
{
	ASSERT(datacount*TiffDatatypeLength(datatype) >= 4); // Usually > 4. However, some tags hold a single offset to a large block of memory (for example stripByteCounts)
	uint32_t value = offset.value();
	unsigned char* bytePointer = reinterpret_cast<unsigned char*>(&value);

	if (e == vibo::GetSystemEndianness())
	{
		m_dataBytes[0] = bytePointer[0];
		m_dataBytes[1] = bytePointer[1];
		m_dataBytes[2] = bytePointer[2];
		m_dataBytes[3] = bytePointer[3];
	}
	else
	{
		m_dataBytes[0] = bytePointer[3];
		m_dataBytes[1] = bytePointer[2];
		m_dataBytes[2] = bytePointer[1];
		m_dataBytes[3] = bytePointer[0];
	}
}


TiffDirEntry::TiffDirEntry(int tagid, int datatype, int datacount, const uint32_t& longvalue, Endianness e) : m_endianness(e), m_tagID(tagid), m_dataType(datatype), m_dataCount(datacount), m_storageLogic(StorageLogic::LongData)
{

	if (!(TiffDatatypeLength(m_dataType) == 4 && m_dataCount == 1))
	{
		ASSERT(false);
	}

	ASSERT(TiffDatatypeLength(m_dataType) == 4 && m_dataCount == 1);

	const unsigned char* bytePointer = reinterpret_cast<const unsigned char*>(&longvalue);

	if (e == vibo::GetSystemEndianness())
	{
		m_dataBytes[0] = bytePointer[0];
		m_dataBytes[1] = bytePointer[1];
		m_dataBytes[2] = bytePointer[2];
		m_dataBytes[3] = bytePointer[3];
	}
	else
	{
		m_dataBytes[0] = bytePointer[3];
		m_dataBytes[1] = bytePointer[2];
		m_dataBytes[2] = bytePointer[1];
		m_dataBytes[3] = bytePointer[0];
	}
}


TiffDirEntry::TiffDirEntry(int tagid, int datatype, int datacount, const AsShort& ts, Endianness e) : m_endianness(e), m_tagID(tagid), m_dataType(datatype), m_dataCount(datacount), m_storageLogic(StorageLogic::ShortData)
{
	ASSERT(TiffDatatypeLength(m_dataType) == 2 && (m_dataCount == 1 || m_dataCount == 2));

	uint16_t first_word  = ts[0];
	uint16_t second_word = ts[1];

	unsigned char* w1 = reinterpret_cast<unsigned char*>(&first_word);
	unsigned char* w2 = reinterpret_cast<unsigned char*>(&second_word);

	if (e == vibo::GetSystemEndianness())
	{
		m_dataBytes[0] = w1[0];
		m_dataBytes[1] = w1[1];
		m_dataBytes[2] = w2[0];
		m_dataBytes[3] = w2[1];
	}
	else
	{
		m_dataBytes[0] = w1[1];
		m_dataBytes[1] = w1[0];
		m_dataBytes[2] = w2[1];
		m_dataBytes[3] = w2[0];
	}
}


TiffDirEntry::TiffDirEntry(int tagid, int datatype, int datacount, const AsByte& fb, Endianness e) : m_endianness(e), m_tagID(tagid), m_dataType(datatype), m_dataCount(datacount), m_storageLogic(StorageLogic::ByteData)
{
	m_dataBytes[0] = fb[0];
	m_dataBytes[1] = fb[1];
	m_dataBytes[2] = fb[2];
	m_dataBytes[3] = fb[3];
}


uint32_t TiffDirEntry::GetOffsetField() const
{
	if (m_storageLogic != StorageLogic::OffsetData)
	{
		ASSERT(false);
	}

	ASSERT(m_storageLogic == StorageLogic::OffsetData);
	if (m_endianness == Endianness::Little)
	{
		return 256 * 256 * 256 * m_dataBytes[3] + 256 * 256 * m_dataBytes[2] + 256 * m_dataBytes[1] + m_dataBytes[0];
	}
	else if (m_endianness == Endianness::Big)
	{
		return 256 * 256 * 256 * m_dataBytes[0] + 256 * 256 * m_dataBytes[1] + 256 * m_dataBytes[2] + m_dataBytes[3];
	}
	ASSERT(false);
	return 0; // never reached
}


uint32_t TiffDirEntry::GetLongValue() const
{
	ASSERT(m_storageLogic == StorageLogic::LongData);
	if (m_endianness == Endianness::Little)
	{
		return 256 * 256 * 256 * m_dataBytes[3] + 256 * 256 * m_dataBytes[2] + 256 * m_dataBytes[1] + m_dataBytes[0];
	}
	else if (m_endianness == Endianness::Big)
	{
		return 256 * 256 * 256 * m_dataBytes[0] + 256 * 256 * m_dataBytes[1] + 256 * m_dataBytes[2] + m_dataBytes[3];
	}
	ASSERT(false);
	return 0; // never reached
}


int TiffDirEntry::GetIntegerValue() const
{
	if (m_storageLogic == StorageLogic::ShortData)
	{
		AsShort ts = GetTwoShorts();
		return ts[0];
	}
	else if (m_storageLogic == StorageLogic::LongData)
	{
		return GetLongValue();
	}
	ASSERT(false);
	return 0; // never reached
}


AsShort TiffDirEntry::GetTwoShorts() const
{
	ASSERT(m_storageLogic == StorageLogic::ShortData);

	AsShort retval;
	if (m_endianness == Endianness::Little)
	{
		retval[0] = 256 * m_dataBytes[1] + m_dataBytes[0];
		retval[1] = 256 * m_dataBytes[3] + m_dataBytes[2];
		return retval;
	}
	else if (m_endianness == Endianness::Big)
	{
		retval[0] = 256 * m_dataBytes[0] + m_dataBytes[1];
		retval[1] = 256 * m_dataBytes[2] + m_dataBytes[3];
		return retval;
	}
	ASSERT(false);
	return retval; // never reached
}


AsByte TiffDirEntry::GetFourBytes() const
{
	AsByte retval;
	retval[0] = m_dataBytes[0];
	retval[1] = m_dataBytes[1];
	retval[2] = m_dataBytes[2];
	retval[3] = m_dataBytes[3];
	return retval;
}


void TiffDirEntry::InitializeFromMemory(const unsigned char* mem, Endianness e)
{
	m_endianness = e;
	m_tagID = vibo::MakeUShort(mem, e);
	m_dataType = vibo::MakeUShort(mem + 2, e);
	m_dataCount = vibo::MakeULong(mem + 4, e);
	m_dataBytes[0] = vibo::MakeUByte(mem + 8);
	m_dataBytes[1] = vibo::MakeUByte(mem + 9);
	m_dataBytes[2] = vibo::MakeUByte(mem + 10);
	m_dataBytes[3] = vibo::MakeUByte(mem + 11);

	int sizeof_datatype = TiffDatatypeLength(m_dataType);
	int sizeof_data = sizeof_datatype * m_dataCount;
	if (sizeof_data > 4)
	{
		m_storageLogic = StorageLogic::OffsetData;
	}
	else
	{
		switch (sizeof_datatype)
		{
		case 4:
		{
			if (m_tagID == TiffTag::ExifIFD || m_tagID == TiffTag::GPSIFD)
			{
				m_storageLogic = StorageLogic::OffsetData;
			}
			else
			{
				m_storageLogic = StorageLogic::LongData;
			}
		} break;

		case 2:
		{
			m_storageLogic = StorageLogic::ShortData;
		} break;

		case 1:
		{
			m_storageLogic = StorageLogic::ByteData;
		} break;

		default:
		{
			THROW(L"Illegal data type size!");
		}
		}
	}
}


void TiffDirEntry::BuildMemoryRepresentation(unsigned char* mem, Endianness e)
{
	vibo::PutUShort(mem, m_tagID, e);
	vibo::PutUShort(mem + 2, m_dataType, e);
	vibo::PutUlong(mem + 4, m_dataCount, e);
	mem[8] = m_dataBytes[0];
	mem[9] = m_dataBytes[1];
	mem[10] = m_dataBytes[2];
	mem[11] = m_dataBytes[3];
}


std::wstring TiffDirEntry::StringRepresentation(Endianness e) const
{
	std::wstringstream ss;
	ss << std::setw(18) << std::left << TiffTagName(m_tagID) << L" " << std::setw(12) << GetDatatypeRepresentation(m_dataType, m_dataCount, e) << std::setw(0) << L" " << GetValueRepresentation(m_dataBytes, m_dataType, m_dataCount, e);
	return ss.str();
}




int TiffDirEntry::Tag() const
{
	return m_tagID;
}


ULong_t TiffDirEntry::GetDataSize() const
{
	return m_dataCount * TiffDatatypeLength(m_dataType);
}


int TiffDirEntry::GetDataType() const
{
	return m_dataType;
}


int TiffDirEntry::GetDataCount() const
{
	return m_dataCount;
}


int TiffDirEntry::GetElementSize() const
{
	return TiffDatatypeLength(m_dataType);
}



std::wstring TiffDataTypeString(int typ)
{
	switch (typ)
	{
	case 1: return L"Ubyte";
	case 2: return L"Ascii";
	case 3: return L"Ushort";
	case 4: return L"Ulong";
	case 5: return L"Rational";
	case 6: return L"Sbyte";
	case 7: return L"Xbyte";
	case 8: return L"Sshort";
	case 9: return L"Slong";
	case 10: return L"SRational";
	case 11: return L"Float";
	case 12: return L"Double";
	case 13: return L"IDF";
	}
	ASSERT(false);
	static const std::wstring Q(L"?");
	return Q;
}


UShort_t TiffDatatypeLength(int typ)
{
	switch (typ)
	{
	case  1: return 1; // L"Ubyte";
	case  2: return 1; // L"Ascii";
	case  3: return 2; // L"Ushort";
	case  4: return 4; // L"Ulong";
	case  5: return 8; // L"Rational";
	case  6: return 1; // L"Sbyte";
	case  7: return 1; // L"Xbyte";
	case  8: return 2; // L"Sshort";
	case  9: return 4; // L"Slong";
	case 10: return 8; // L"SRational";
	case 11: return 4; // L"Float";
	case 12: return 8; // L"Double";
	}
	ASSERT(false);
	return 0;
}


std::wstring Hex(int v)
{
	if (v > 255 || v < 0)
	{
		return std::wstring(L"??");
	}
	int lo = (v & 15);
	int hi = (v / 16);
	static const char* hh = "0123456789abcdef";
	wchar_t t[3];
	t[0] = hh[hi];
	t[1] = hh[lo];
	t[2] = L'\0';
	return std::wstring(t);
}


std::wstring GetSingleValueRepresentation(const unsigned char* data, int dataType, Endianness e)
{
	switch(dataType)
	{
	case  1: return std::to_wstring(vibo::MakeUByte(data)); // L"Ubyte";
	case  2: return std::to_wstring(vibo::MakeUByte(data)); // L"Ascii";
	case  3: return std::to_wstring(vibo::MakeUShort(data, e)); // L"Ushort";
	case  4: return std::to_wstring(vibo::MakeULong(data, e)); // L"Ulong";
	case  6: return std::to_wstring(vibo::MakeSByte(data)); // L"Sbyte";
	case  7: return std::to_wstring(vibo::MakeUByte(data)); // L"Xbyte";
	case  8: return std::to_wstring(vibo::MakeSShort(data, e)); // L"Sshort";
	case  9: return std::to_wstring(vibo::MakeSLong(data, e)); // L"Slong";
	default: break;
	}

	std::wstring tmp(L":");
	tmp += Hex(data[0]);
	tmp += L" ";
	tmp += Hex(data[1]);
	tmp += L" ";
	tmp += Hex(data[2]);
	tmp += L" ";
	tmp += Hex(data[3]);
	tmp += L"]";

	switch (dataType)
	{
	case  5: return std::wstring(L"[Rational") + tmp; // L"Rational";
	case 10: return std::wstring(L"[SRational")+tmp; // L"SRational";
	case 11: return std::wstring(L"[Float") + tmp; // L"Float";
	case 12: return std::wstring(L"[Double") + tmp; // L"Double";
	default: break;
	}
	ASSERT(false);
	static const std::wstring Q(L"?");
	return Q;
}


std::wstring GetValuePairRepresentation(const unsigned char* data, int dataType, Endianness e)
{
	ASSERT(TiffDatatypeLength(dataType) == 2);

	unsigned char data1[4];
	unsigned char data2[4];
	
	data1[0] = data[0];
	data1[1] = data[1];
	data1[2] = 0;
	data1[3] = 0;

	data2[0] = data[2];
	data2[1] = data[3];
	data2[2] = 0;
	data2[3] = 0;
	std::wstring s1 = GetSingleValueRepresentation(data, dataType, e);
	std::wstring s2 = GetSingleValueRepresentation(data2, dataType, e);
	std::wstring retval = L"(";
	retval += s1;
	retval += L", ";
	retval += s2;
	retval += L")";
	return retval;
}


std::wstring GetByteDataRepresentation(const unsigned char* data, int dataType, int dataCount, Endianness e)
{
	std::wstring representation{L"("};
	
	for (int i = 0; i < dataCount; ++i)
	{
		if (i > 0)
		{
			representation += L", ";
		}
		representation += std::to_wstring(int(data[i]));
	}
	representation += L")";
	return representation;
}


std::wstring GetValueRepresentation(const unsigned char* dataBytes, int dataType, int datacount, Endianness e)
{
	std::wstring valueRepresentation;

	if (TiffDatatypeLength(dataType) <= 4 && datacount == 1)
	{
		valueRepresentation = GetSingleValueRepresentation(dataBytes, dataType, e);
	}
	else if (TiffDatatypeLength(dataType) == 2 && datacount == 2)
	{
		valueRepresentation = GetValuePairRepresentation(dataBytes, dataType, e);
	}
	else if (TiffDatatypeLength(dataType) == 1 && datacount <= 4)
	{
		valueRepresentation = GetByteDataRepresentation(dataBytes, dataType, datacount, e);
	}
	else
	{	// Assume offset
		valueRepresentation = L"[Offs:";
		valueRepresentation += GetSingleValueRepresentation(dataBytes, Ulong, e);
		valueRepresentation += L"]";
	}
	return valueRepresentation;
}


std::wstring GetDatatypeRepresentation(int datatype, int datacount, Endianness e)
{
	std::wstring representation = TiffDataTypeString(datatype);
	if (datacount > 1)
	{
		representation += L'[';
		representation += std::to_wstring(datacount);
		representation += L']';
	}
	return representation;
}

//	_________________________________________________________________________________________________________________________________________________
//
//			TiffTagName() -- uses preprocessor macro expansion to populate the cases of the switch statement
//	_________________________________________________________________________________________________________________________________________________


std::wstring TiffTagName(int ID)
{
	switch (ID)
	{
#define TIFFTAG_MACRO(name, dir, type, value)  case value: return L#name;
#include "TiffTags.hxx"
#undef  TIFFTAG_MACRO
	}
	std::wstring retval(L"ID:");
	retval += std::to_wstring(ID);
	retval += L"  ";
	return retval;
}


