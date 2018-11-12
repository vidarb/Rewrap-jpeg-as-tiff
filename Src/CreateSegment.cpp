// File: CreateSegment.cpp
// This file is part of the project: Rewrap-jpeg-as-tiff -- Rewraps a jpeg file into a TIFF container, without loss of information (no re-encoding).
// Copyright(c) 2018 Vidar Bosnes
// This software is licenced under the GNU GENERAL PUBLIC LICENSE Version 3

#include "FileSegment.h"
#include "TiffSegments.h"
#include "JpegSegments.h"
#include "Exception.h"
#include <map>
#include <typeindex>
#include <typeinfo>
#include <memory>

// --------------------------------------------------------------------------------------------------------------------
//		Reverse map from FileSegment type_index to Segmenttype and wstring (== name)
// --------------------------------------------------------------------------------------------------------------------

class ReverseSegmenttypeMap
{
	std::map<std::type_index, std::tuple<Segmenttype, std::wstring>> m_map;

public:
	static ReverseSegmenttypeMap& GetInstance();
	void Insert(const std::type_index& typ, Segmenttype seg, const std::wstring& str);
	Segmenttype LookupSegmenttype(const std::type_index& typ);
	std::wstring LookupString(const std::type_index& typ);

private:
	ReverseSegmenttypeMap() = default;
	~ReverseSegmenttypeMap() = default;
	ReverseSegmenttypeMap(const ReverseSegmenttypeMap&) = delete;
	ReverseSegmenttypeMap& operator=(const ReverseSegmenttypeMap&) = delete;
};


// --------------------------------------------------------------------------------------------------------------------
//		ReverseSegmenttypeMap implementation
// --------------------------------------------------------------------------------------------------------------------

ReverseSegmenttypeMap& ReverseSegmenttypeMap::GetInstance()
{
	static ReverseSegmenttypeMap theMap;
	return theMap;
}


void ReverseSegmenttypeMap::Insert(const std::type_index& typ, Segmenttype seg, const std::wstring& str)
{
	ASSERT(seg != Segmenttype::Undefined); // Illegal to insert type info associated with Undefined segment
	Segmenttype s2 = LookupSegmenttype(typ);
	if (s2 == Segmenttype::Undefined)
	{
		std::type_index idx(typ);
		auto result = m_map.insert(std::make_pair(idx, std::make_tuple(seg, str)));
		ASSERT(result.second == true); // true indicates that a new element was inserted, which should be the case since lookup returned Undefined
	}
}


Segmenttype ReverseSegmenttypeMap::LookupSegmenttype(const std::type_index& typ)
{
	std::type_index idx(typ);
	auto it = m_map.find(idx);
	if (it == m_map.end())
	{
		return Segmenttype::Undefined;
	}
	return std::get<Segmenttype>(it->second);
}


std::wstring  ReverseSegmenttypeMap::LookupString(const std::type_index& typ)
{
	std::type_index idx(typ);
	auto it = m_map.find(idx);
	if (it == m_map.end())
	{
		return L"Undefined!";
	}
	return std::get<std::wstring>(it->second);
}


// --------------------------------------------------------------------------------------------------------------------
//		CreateSegment()
// --------------------------------------------------------------------------------------------------------------------

namespace
{
	std::tuple<const wchar_t*, FileSegment*> CreateSegment_local(Segmenttype seg, Endianness e, Offset_t offset, int size)
	{
		switch (seg)
		{
		case Segmenttype::JpegStartOfImage:		 return std::make_tuple(L"JpegStartOfImage", new JpegStartOfImage(offset, size, e));
		case Segmenttype::JpegEndOfImage:		 return std::make_tuple(L"JpegEndOfImage", new JpegEndOfImage(offset, size, e));
		case Segmenttype::JpegRestartMarker:     return std::make_tuple(L"JpegRestartMarker", new JpegRestartMarker(offset, size, e));
		case Segmenttype::JpegApp0Segment:       return std::make_tuple(L"JpegApp0Segment", new JpegApp0Segment(offset, size, e));
		case Segmenttype::JpegApp1Segment:       return std::make_tuple(L"JpegApp1Segment", new JpegApp1Segment(offset, size, e));
		case Segmenttype::JpegApp2Segment:       return std::make_tuple(L"JpegApp2Segment", new JpegApp2Segment(offset, size, e));
		case Segmenttype::JpegApp3Segment:       return std::make_tuple(L"JpegApp3Segment", new JpegApp3Segment(offset, size, e));
		case Segmenttype::JpegOtherAppSegment:	 return std::make_tuple(L"JpegOtherAppSegment", new JpegOtherAppSegment(offset, size, e));
		case Segmenttype::JpegQuantizationTable: return std::make_tuple(L"JpegQuantizationTable", new JpegQuantizationTable(offset, size, e));
		case Segmenttype::JpegStartOfFrame:      return std::make_tuple(L"JpegStartOfFrame", new JpegStartOfFrame(offset, size, e));
		case Segmenttype::JpegHuffmanTable:      return std::make_tuple(L"JpegHuffmanTable", new JpegHuffmanTable(offset, size, e));
		case Segmenttype::JpegStartOfScan:       return std::make_tuple(L"JpegStartOfScan", new JpegStartOfScan(offset, size, e));
		case Segmenttype::JpegImageData:         return std::make_tuple(L"JpegImageData", new JpegImageData(offset, size, e));
		case Segmenttype::JpegNumberOfLines:	 return std::make_tuple(L"JpegNumberOfLines", new JpegNumberOfLines(offset, size, e));
		case Segmenttype::JpegRestartInterval: 	 return std::make_tuple(L"JpegRestartInterval", new JpegRestartInterval(offset, size, e));
		case Segmenttype::JpegSpecialSegment:	 return std::make_tuple(L"JpegSpecialSegment", new JpegSpecialSegment(offset, size, e));
		case Segmenttype::JpegCommentSegment:	 return std::make_tuple(L"JpegCommentSegment", new JpegCommentSegment(offset, size, e));
		case Segmenttype::JpegReservedSegment:	 return std::make_tuple(L"JpegReservedSegment", new JpegReservedSegment(offset, size, e));
		case Segmenttype::JpegUnknownSegment:	 return std::make_tuple(L"JpegUnknownSegment", new JpegUnknownSegment(offset, size, e));
		case Segmenttype::TiffHeader:			 return std::make_tuple(L"TiffHeader", new TiffHeader(offset, size, e));
		case Segmenttype::TiffDirectory:		 return std::make_tuple(L"TiffDirectory", new TiffDirectory(offset, size, e));
		case Segmenttype::TiffImageData:		 return std::make_tuple(L"TiffImageData", new TiffImageData(offset, size, e));
		case Segmenttype::TiffByteVector:		 return std::make_tuple(L"TiffByteVector", new TiffByteVector(offset, size, e));
		case Segmenttype::TiffUShortVector:		 return std::make_tuple(L"TiffUShortVector", new TiffUShortVector(offset, size, e));
		case Segmenttype::TiffOffsetTable:		 return std::make_tuple(L"TiffOffsetTable", new TiffOffsetTable(offset, size, e));
		case Segmenttype::TiffBytecountTable:	 return std::make_tuple(L"TiffBytecountTable", new TiffBytecountTable(offset, size, e));
		case Segmenttype::Padding:				 return std::make_tuple(L"Padding", new Padding(offset, size, e));

		default: break;
		}
		ASSERT(false); // Unable to create object referred to by 'seg'
		return std::make_tuple<const wchar_t*, FileSegment*>(nullptr, nullptr); // never reached.
	}
} // end anonymous namespace



std::shared_ptr<FileSegment> CreateSegment(Segmenttype seg, Endianness e, Offset_t offset, int size)
{
	static ReverseSegmenttypeMap& R = ReverseSegmenttypeMap::GetInstance();

	ASSERT(seg != Segmenttype::Undefined);
	auto tu = CreateSegment_local(seg, e, offset, size);
	FileSegment* p = std::get<FileSegment*>(tu);
	ASSERT(p != nullptr);
	const FileSegment& fs = *p;
	std::type_index idx = typeid(fs);
	std::wstring str = std::get<const wchar_t*>(tu);

	R.Insert(idx, seg, str);
	ASSERT(R.LookupSegmenttype(idx) == seg);
	return std::shared_ptr<FileSegment>(p);
}

// --------------------------------------------------------------------------------------------------------------------
//		GetSegmenttype()
// --------------------------------------------------------------------------------------------------------------------

Segmenttype GetSegmenttype(const FileSegment& fs)
{
	static ReverseSegmenttypeMap& R = ReverseSegmenttypeMap::GetInstance();

	std::type_index idx = typeid(fs);
	Segmenttype seg = R.LookupSegmenttype(idx);
	ASSERT(seg != Segmenttype::Undefined);
	return seg;
}


// --------------------------------------------------------------------------------------------------------------------
//		GetSegmentName()
// --------------------------------------------------------------------------------------------------------------------

std::wstring GetSegmentName(const FileSegment& fs)
{
	static ReverseSegmenttypeMap& R = ReverseSegmenttypeMap::GetInstance();

	std::type_index idx = typeid(fs);
	std::wstring str = R.LookupString(idx);
	return str;
}

