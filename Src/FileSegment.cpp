// File: FileSegment.cpp
// This file is part of the project: Rewrap-jpeg-as-tiff -- Rewraps a jpeg file into a TIFF container, without loss of information (no re-encoding).
// Copyright(c) 2018 Vidar Bosnes
// This software is licenced under the GNU GENERAL PUBLIC LICENSE Version 3

#include "FileSegment.h"
#include "Exception.h"
#include "Util.h"
#include "CreateSegment.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include "GetMD5Hash.h"

// --------------------------------------------------------------------------------------------------------------------
//		class FileSegment
// --------------------------------------------------------------------------------------------------------------------

FileSegment::FileSegment(int offset, int size) : m_offset(offset), m_size(size), m_data(), m_label()
{
}


Segmenttype FileSegment::GetSegmenttype() const
{
	return ::GetSegmenttype(*this);
}


int FileSegment::GetSize() const
{
	int datasiz = vibo::size(m_data);
	if (m_size != datasiz)
	{
		ASSERT(false);
	}
	ASSERT(m_size == datasiz);
	return  m_size;
}

int FileSegment::GetOffset() const
{
	return m_offset;
}


void FileSegment::SetOffset(int offset)
{
	m_offset = offset;
}


void FileSegment::ReadData(FILE* f)
{
	ASSERT(m_size > 0);
	int check = fseek(f, m_offset, SEEK_SET);
	ASSERT(check == 0);
	m_data = vibo::GetBytes(f, m_size);
	InterpretData();
}


void FileSegment::RebuildBinaryData()
{
	std::wstring msg = L"RebuildBinaryData() is not implemented for ";
	std::string tmp = typeid(*this).name();
	msg += vibo::to_wstring(tmp);
	msg += L".";
	THROW(msg);
}


int FileSegment::GetDataByte(int idx) const
{
	ASSERT(vibo::size(m_data) > idx);
	return m_data[idx];
}


void FileSegment::SetLabel(const std::wstring& label)
{
	m_label = label;
}


std::wstring FileSegment::Label() const
{
	return m_label;
}


bool FileSegment::HasLabel() const
{
	return (m_label.length() > 0);
}


void FileSegment::InterpretData()
{
	// No action in base class
}


std::shared_ptr<FileSegment> FileSegment::Clone()
{
	std::shared_ptr<FileSegment> theclone = CreateSegment(GetSegmenttype(), FileEndianness(), GetOffset(), GetSize());
	theclone->m_data = this->m_data;
	theclone->InterpretData();
	return theclone;
}

void FileSegment::Dump() const
{
	std::vector<std::wstring> vec = StringRepresentation();
	for (auto it = vec.begin(); it != vec.end(); ++it)
	{
		std::wcout << *it << std::endl;
	}
}


std::vector<std::wstring> FileSegment::StringRepresentation() const
{
	Segmenttype seg = GetSegmenttype();

	std::wstring title = GetSegmentName(*this);
	if (HasLabel())
	{
		title += L'(';
		title += Label();
		title += L')';
	}

	std::wstringstream s;
	s << std::dec << std::setfill(L'0') << std::setw(8) << std::right << m_offset << std::setfill(L' ') << L" " << std::setw(0) << title << " Size:" << m_data.size();

	std::vector<std::wstring> retval;
	retval.push_back(s.str());
	
	if (!m_data.empty())
	{
		std::wstring md5str = vibo::GetMD5Hash(m_data);
		retval.push_back(md5str);
	}
	if (GetSegmenttype() == Segmenttype::JpegStartOfFrame)
	{
		std::wstringstream s2;
		s2 << L"DEBUG Start of frame: " << std::hex << m_data[0] << L" " << m_data[1];
	}
	return retval;
}


void FileSegment::WriteToFile(FILE* f) const
{
	int siz = vibo::size(m_data);
	ASSERT(siz == m_size);
	size_t check = fwrite(&m_data[0], 1, m_size, f);
	ASSERT(check == m_size);
}


const ByteVector& FileSegment::Data() const
{
	return m_data;
}


// --------------------------------------------------------------------------------------------------------------------
//		class Padding
// --------------------------------------------------------------------------------------------------------------------

Padding::Padding(int offset, int size, Endianness e) : FileSegment(offset, size), m_Endianness(e)
{
	m_data.resize(size);
	memset(&m_data[0], 0, size);
}

Endianness Padding::FileEndianness() const
{
	return this->m_Endianness;
}


