// File: ConvertJpegToTiff.cpp
// This file is part of the project: Rewrap-jpeg-as-tiff -- Rewraps a jpeg file into a TIFF container, without loss of information (no re-encoding).
// Copyright(c) 2018 Vidar Bosnes
// This software is licenced under the GNU GENERAL PUBLIC LICENSE Version 3

#pragma warning(disable: 4996)

#include "ConvertJpegToTiff.h"
#include "GraphicsFile.h"
#include "TiffSegments.h"
#include "JpegSegments.h"
#include "CreateSegment.h"
#include "Exception.h"
#include <iostream>
#include "ReadJpegMetadata.h"
#include <memory>


std::shared_ptr<TiffHeader> FindTiffHeader(std::vector<std::shared_ptr<FileSegment>>& vec)
{
	for (auto it = vec.begin(); it != vec.end(); ++it)
	{
		if ((*it)->GetSegmenttype() == Segmenttype::TiffHeader)
		{
			std::shared_ptr<TiffHeader> hdr = std::dynamic_pointer_cast<TiffHeader>(*it);
			ASSERT(hdr != nullptr);
			return hdr;
		}
	}
	ASSERT(false);
	return std::shared_ptr<TiffHeader>{}; // Not reached
}


ByteVector change_endianness(const ByteVector& data, int elementsize)
{
	if (elementsize == 1)
	{
		return data;
	}
	int size = vibo::size(data);
	ByteVector retval(size);
	for (int i = 0; i < size; i += elementsize)
	{
		for (int j = 0; j < elementsize; ++j)
		{
			retval[i + j] = data[i + elementsize - j - 1];
		}
	}
	return retval;
}


std::shared_ptr<FileSegment> MakeTiffHeader(Endianness e, Offset_t offset)
{
	std::shared_ptr<FileSegment> S = CreateSegment(Segmenttype::TiffHeader, e, offset, 8);
	S->RebuildBinaryData();
	return S;
}


std::shared_ptr<FileSegment> MakeJpegStartOfImage(Offset_t offset)
{
	std::shared_ptr<FileSegment> S = CreateSegment(Segmenttype::JpegStartOfImage, Endianness::Big, offset, 2); // Jpeg always Big-endian
	ASSERT(S != nullptr);
	S->RebuildBinaryData();
	return S;
}


std::shared_ptr<FileSegment> MakeJpegEndOfImage(Offset_t offset)
{
	std::shared_ptr<FileSegment> S = CreateSegment(Segmenttype::JpegEndOfImage, Endianness::Big, offset, 2); // Jpeg always Big-endian
	ASSERT(S != nullptr);
	S->RebuildBinaryData();
	return S;
}


// --------------------------------------------------------------------------------------------------------------------
//		Write_Selected_Entries()
// --------------------------------------------------------------------------------------------------------------------

typedef bool selector_function(int, int);

// Writes data corresponding to the entries to outfile if sizeof(data) > 4
// Stores TIFF directory entries in dir_entries, which is returned, and must be written by the caller.

std::vector<TiffDirEntry> Write_Selected_Entries(std::vector<std::tuple<TiffDirEntry, ByteVector>>& dir_info, GraphicsVector& outfile, Endianness exif_endianness, Endianness outfile_endianness, selector_function foo)
{
	std::vector<TiffDirEntry> dir_entries;
	Offset_t offset = outfile.back()->GetOffset() + outfile.back()->GetSize();

	if (vibo::size(dir_info) > 0)
	{
		// Write data, build vector of TiffDirEntries

		for (int i = 0; i < vibo::size(dir_info); ++i)
		{
			TiffDirEntry& E = std::get<TiffDirEntry>(dir_info[i]);
			ByteVector  V = std::get<ByteVector>(dir_info[i]); // NOT reference!
			std::vector<TiffDirEntry> vv;

			int tag = E.Tag();
			int datatype = E.GetDataType();
			int datacount = E.GetDataCount();
			int datasize = E.GetDataSize();
			int elementsize = E.GetElementSize();

			if (foo(tag, datatype))
			{
				if (datasize > 4)
				{
					if (outfile_endianness != exif_endianness)
					{
						int elementsize_for_change_endianness = elementsize;
						if (datatype == Datatype::Rational || datatype == Datatype::SRational)
						{
							// Rationals are 8 bytes, but consist of two values. When changing endianness treat as 4 bytes!
							elementsize_for_change_endianness = 4;
						}
						V = change_endianness(V, elementsize_for_change_endianness);
					}
					Offset_t data_offset = offset;
					std::shared_ptr<FileSegment> S = CreateSegment(Segmenttype::TiffByteVector, outfile_endianness, offset, datasize);
					std::shared_ptr<TiffByteVector> bv = std::dynamic_pointer_cast<TiffByteVector>(S);
					bv->assign(V);
					offset = AddSegmentPadded(outfile, S);
					TiffDirEntry e(tag, datatype, datacount, AsOffset(data_offset), outfile_endianness);
					dir_entries.push_back(e);
				}
				else if (elementsize == 2 && datacount == 2)
				{
					AsShort ts = E.GetTwoShorts();
					TiffDirEntry e(tag, E.GetDataType(), E.GetDataCount(), ts, outfile_endianness);
					dir_entries.push_back(e);
				}
				else if (elementsize == 1)
				{
					AsByte fb = E.GetFourBytes();
					TiffDirEntry e(tag, E.GetDataType(), E.GetDataCount(), fb, outfile_endianness);
					dir_entries.push_back(e);
				}
				else if (elementsize == 2)// datacount == 1, elementsize == 4 or 2
				{
					AsShort ts = E.GetTwoShorts();
					TiffDirEntry e(tag, E.GetDataType(), E.GetDataCount(), ts, outfile_endianness);
					dir_entries.push_back(e);
				}
				else if (elementsize == 4)
				{
					uint32_t longval = E.GetLongValue();
					TiffDirEntry e(tag, E.GetDataType(), E.GetDataCount(), longval, outfile_endianness);
					dir_entries.push_back(e);
				}
				else
				{
					ASSERT(false);
				}

			}
		}
	}
	return dir_entries;
}


// --------------------------------------------------------------------------------------------------------------------
//		Selector functions
// --------------------------------------------------------------------------------------------------------------------

bool relevant_gps_tags(int tag, int datatype)
{
	if (tag == TiffTag::SubIFDs) return false;
	if (tag == TiffTag::InteroperabilityIFD) return false;
	return true;
}


bool relevant_exif_tags(int tag, int datatype)
{
	if (tag == TiffTag::SubIFDs) return false;
	if (tag == TiffTag::MakerNote) return false;
	if (tag == TiffTag::ExifPixelXDimension) return false;
	if (tag == TiffTag::ExifPixelYDimension) return false;
	if (tag == TiffTag::InteroperabilityIFD) return false;

	return true;
}


bool relevant_main_directory_tags(int tag, int datatype)
{
	if (tag == TiffTag::SubIFDs || tag == TiffTag::InteroperabilityIFD)
	{
		return false;
	}
	if (tag == TiffTag::Orientation || tag == TiffTag::Exposure)
	{
		return true;
	}
	return datatype == Datatype::Ascii;
}


// --------------------------------------------------------------------------------------------------------------------
//		Convert Jpeg to TIFF
// --------------------------------------------------------------------------------------------------------------------

void ConvertJpegToTiff(GraphicsVector& G, std::wstring& outfilename)
{
	Endianness TiffFileEndianness = Endianness::Little;

	// Check if the GraphicsVector contains a jpeg image

	auto check_jpeg = G.begin();
	if ((*check_jpeg)->GetSegmenttype() != Segmenttype::JpegStartOfImage)
	{
		THROW(L"Error: the input file was not a JPEG image!");
	}

	// Check if the start-of-frame segment is a baseline DCT segment (marker: ff c0)

	for (auto it = G.begin(); it != G.end(); ++it)
	{
		Segmenttype seg = (*it)->GetSegmenttype();
		if (seg == Segmenttype::JpegStartOfFrame)
		{
			int b1 = (*it)->GetDataByte(0);
			int b2 = (*it)->GetDataByte(1);
			ASSERT(b1 == 0xff);
			if (b2 != 0xc0)
			{
				THROW(L"Sorry, this JPEG cannot be processed. The start-of-frame marker needs to be ff c0 (baseline DCT).");
			}
		}
	}

	std::vector<std::shared_ptr<FileSegment>> TiffFile;
	Offset_t offset = 0;

	// ____________________________________________________________________________________________________________________________________
	//
	//		TIFF HEADER
	// ____________________________________________________________________________________________________________________________________

	std::shared_ptr<FileSegment> hdr = MakeTiffHeader(TiffFileEndianness, offset);
	offset = AddSegmentPadded(TiffFile, hdr);

	// ____________________________________________________________________________________________________________________________________
	//
	//		EMBEDDED IMAGE
	// ____________________________________________________________________________________________________________________________________

	Offset_t embedded_image_offset = offset;

	int imageWidth = 0;
	int imageLength = 0;
	int numComponents = 0;
	int16_t bitsPerSample = 0;

	int verticalSampleFactor_Y = 0;
	int horizontalSampleFactor_Y = 0;
	int verticalSampleFactor_Cb = 0;
	int horizontalSampleFactor_Cb = 0;
	int verticalSampleFactor_Cr = 0;
	int horizontalSampleFactor_Cr = 0;

	std::shared_ptr<FileSegment> soi = MakeJpegStartOfImage(offset);
	offset = AddSegmentPadded(TiffFile, soi);


	for (auto it = G.begin(); it != G.end(); ++it)
	{
		Segmenttype seg = (*it)->GetSegmenttype();

		if (seg == Segmenttype::JpegStartOfFrame || seg == Segmenttype::JpegStartOfScan || seg == Segmenttype::JpegRestartInterval || seg == Segmenttype::JpegImageData)
		{
			std::shared_ptr<FileSegment> S = (*it)->Clone();
			if (seg == Segmenttype::JpegStartOfFrame)
			{
				std::shared_ptr<JpegStartOfFrame> sof = std::dynamic_pointer_cast<JpegStartOfFrame>(S);
				ASSERT(sof != nullptr);

				sof->SetOffset(offset);
				imageWidth = sof->GetImageWidth();
				imageLength = sof->GetImageLength();
				bitsPerSample = sof->GetPrecision();
				numComponents = sof->GetNumComponents();
				if (numComponents > 2)
				{
					horizontalSampleFactor_Y = sof->GetHorizontalSamplingFactor(0);
					horizontalSampleFactor_Cb = sof->GetHorizontalSamplingFactor(1);
					horizontalSampleFactor_Cr = sof->GetHorizontalSamplingFactor(2);
					verticalSampleFactor_Y = sof->GetVerticalSamplingFactor(0);
					verticalSampleFactor_Cb = sof->GetVerticalSamplingFactor(1);
					verticalSampleFactor_Cr = sof->GetVerticalSamplingFactor(2);
				}
			}
			S->SetOffset(offset);
			offset = AddSegmentPadded(TiffFile, S);
		}
	}

	std::shared_ptr<FileSegment> eoi = MakeJpegEndOfImage(offset);
	offset = AddSegmentPadded(TiffFile, eoi);

	Offset_t embedded_image_end = offset;

	Offset_t jpeg_tables_start = offset;

	// ____________________________________________________________________________________________________________________________________
	//
	//		JPEG TABLES
	// ____________________________________________________________________________________________________________________________________

	std::shared_ptr<FileSegment> soi2 = MakeJpegStartOfImage(offset);
	offset = AddSegmentPadded(TiffFile, soi2);

	for (auto it = G.begin(); it != G.end(); ++it)
	{
		Segmenttype seg = (*it)->GetSegmenttype();

		if (seg ==Segmenttype::JpegQuantizationTable || seg == Segmenttype::JpegHuffmanTable)
		{
			std::shared_ptr<FileSegment> S = (*it)->Clone();
			S->SetOffset(offset);
			offset = AddSegmentPadded(TiffFile, S);
		}
	}

	std::shared_ptr<FileSegment> eoi2 = MakeJpegEndOfImage(offset);
	offset = AddSegmentPadded(TiffFile, eoi2);

	Offset_t jpeg_tables_end = offset;

	// ____________________________________________________________________________________________________________________________________
	//
	//		ICC PROFILE
	// ____________________________________________________________________________________________________________________________________

	Offset_t icc_profile_begin = offset;

	std::vector<std::shared_ptr<JpegApp2Segment>> App2Segments;
	ByteVector ICCProfile;
	for (auto it = G.begin(); it != G.end(); ++it)
	{
		Segmenttype seg = (*it)->GetSegmenttype();
		
		if (seg == Segmenttype::JpegApp2Segment)
		{
			std::shared_ptr<FileSegment> S = (*it)->Clone();
			std::shared_ptr<JpegApp2Segment> P = std::dynamic_pointer_cast<JpegApp2Segment>(S);
			if (P != nullptr)
			{
				App2Segments.push_back(P);
			}
		}
	}
	if (vibo::size(App2Segments) > 0)
	{
		ICCProfile = ReadIccProfile(App2Segments);
	}

	if (vibo::size(ICCProfile) > 0)
	{
		std::shared_ptr<FileSegment> S = CreateSegment(Segmenttype::TiffByteVector, TiffFileEndianness, offset, 0);
		std::shared_ptr<TiffByteVector> bv = std::dynamic_pointer_cast<TiffByteVector>(S);
		ASSERT(bv != nullptr);
		bv->assign(ICCProfile);
		bv->RebuildBinaryData();
		offset = AddSegmentPadded(TiffFile, S);
	}

	Offset_t icc_profile_end = offset;

	// ____________________________________________________________________________________________________________________________________
	//
	//		APP 1 METADATA
	// ____________________________________________________________________________________________________________________________________

	std::vector<std::shared_ptr<JpegApp1Segment>> App1Segments;

	for (auto it = G.begin(); it != G.end(); ++it)
	{
		Segmenttype seg = (*it)->GetSegmenttype();

		if (seg == Segmenttype::JpegApp1Segment)
		{
			std::shared_ptr<FileSegment> S = (*it)->Clone();
			std::shared_ptr<JpegApp1Segment> P = std::dynamic_pointer_cast<JpegApp1Segment>(S);
			if (P != nullptr)
			{
				App1Segments.push_back(P);
			}
		}
	}

	exif_info Exif_Info;

	if (vibo::size(App1Segments) > 0)
	{
		Exif_Info = ReadApp1Metadata(App1Segments);
	}
	Endianness exif_endianness = Exif_Info.endianness;

	ASSERT(offset == TiffFile.back()->GetOffset() + TiffFile.back()->GetSize());

	// Write exif directory

	std::vector<std::tuple<TiffDirEntry, ByteVector>> exif_dir = Exif_Info.exif_dir;
	Offset_t exifdir_offset = 0;
	if (vibo::size(exif_dir) > 0)
	{
		auto vec = Write_Selected_Entries(exif_dir, TiffFile, exif_endianness, TiffFileEndianness, relevant_exif_tags);
		offset = TiffFile.back()->GetOffset() + TiffFile.back()->GetSize();
		
		exifdir_offset = offset;
		std::shared_ptr<FileSegment> S = CreateSegment(Segmenttype::TiffDirectory, TiffFileEndianness, offset, 0);
		std::shared_ptr<TiffDirectory> exifdir = std::dynamic_pointer_cast<TiffDirectory> (S);
		for (auto it = vec.begin(); it != vec.end(); ++it)
		{
			exifdir->AddEntry(*it);
		}
		exifdir->RebuildBinaryData();
		offset = AddSegmentPadded(TiffFile, S);
	}

	// Write GPS directory

	std::vector<std::tuple<TiffDirEntry, ByteVector>> gps_dir = Exif_Info.gps_dir;
	Offset_t gpsdir_offset = 0;
	if (vibo::size(gps_dir) > 0)
	{
		auto vec = Write_Selected_Entries(gps_dir, TiffFile, exif_endianness, TiffFileEndianness, relevant_gps_tags);
		offset = TiffFile.back()->GetOffset() + TiffFile.back()->GetSize();

		gpsdir_offset = offset;

		std::shared_ptr<FileSegment> S = CreateSegment(Segmenttype::TiffDirectory, TiffFileEndianness, offset, 0);
		std::shared_ptr<TiffDirectory> gpsdir_ptr = std::dynamic_pointer_cast<TiffDirectory> (S);

		for (auto it = vec.begin(); it != vec.end(); ++it)
		{
			gpsdir_ptr->AddEntry(*it);
		}
		gpsdir_ptr->RebuildBinaryData();
		offset = AddSegmentPadded(TiffFile, S);
	}

	// Write the external data corresponding to relevant entries in the  jpeg's exif main directory
	// The return value will be inserted in the main TIFF directory of the output image

	std::vector<std::tuple<TiffDirEntry, ByteVector>> main_dir = Exif_Info.main_dir;
	std::vector<TiffDirEntry> main_dir_entries_from_exif;
	if (vibo::size(main_dir) > 0)
	{
		main_dir_entries_from_exif = Write_Selected_Entries(main_dir, TiffFile, exif_endianness, TiffFileEndianness, relevant_main_directory_tags);
		offset = TiffFile.back()->GetOffset() + TiffFile.back()->GetSize();
	}

	// ____________________________________________________________________________________________________________________________________
	//
	//		TIFF DIRECTORY
	// ____________________________________________________________________________________________________________________________________

	ASSERT(numComponents == 1 || numComponents > 2); // We do not allow 2 components!

	Offset_t bitsPerSample_offset = offset;

	if (numComponents > 2)
	{
		std::shared_ptr<FileSegment> S = CreateSegment(Segmenttype::TiffUShortVector, TiffFileEndianness, offset, 0);
		std::shared_ptr<TiffUShortVector> usv = std::dynamic_pointer_cast<TiffUShortVector>(S);
		ASSERT(usv != nullptr);
		for (int i = 0; i < numComponents; ++i)
		{
			usv->push_back(bitsPerSample);
		}
		offset = AddSegmentPadded(TiffFile, S);
	}

	// Update header to point to main TIFF directory
	
	std::shared_ptr<TiffHeader> hh = FindTiffHeader(TiffFile);
	hh->SetDirectoryOffset(offset);
	hh->RebuildBinaryData();

	std::shared_ptr<FileSegment> S = CreateSegment(Segmenttype::TiffDirectory, TiffFileEndianness, offset, 0);
	std::shared_ptr<TiffDirectory> tiffdir = std::dynamic_pointer_cast<TiffDirectory> (S);
	ASSERT(tiffdir != nullptr);

	TiffDirEntry e1(TiffTag::ImageWidth, Datatype::Ulong, 1, imageWidth, TiffFileEndianness);
	tiffdir->AddEntry(e1);
	TiffDirEntry e2(TiffTag::ImageLength, Datatype::Ulong, 1, imageLength, TiffFileEndianness);
	tiffdir->AddEntry(e2);
	if (numComponents > 2)
	{
		TiffDirEntry e3(TiffTag::BitsPerSample, Datatype::Ushort, 3, AsOffset(bitsPerSample_offset), TiffFileEndianness);
		tiffdir->AddEntry(e3);
	}
	else if (numComponents == 1)
	{
		TiffDirEntry e3(TiffTag::BitsPerSample, Datatype::Ushort, 1, AsShort(bitsPerSample), TiffFileEndianness);
		tiffdir->AddEntry(e3);
	}
	TiffDirEntry e4(TiffTag::Compression, Datatype::Ushort, 1, AsShort(7), TiffFileEndianness);
	tiffdir->AddEntry(e4);

	int photometric = 0;
	if (numComponents == 1)
	{
		photometric = 1; // Min is black
	}
	else
	{
		photometric = 6; // YCbCr
	}
	TiffDirEntry e5(TiffTag::PhotometricInterpretation, Datatype::Ushort, 1, AsShort(photometric), TiffFileEndianness);
	tiffdir->AddEntry(e5);

	TiffDirEntry e6(TiffTag::StripOffsets, Datatype::Ulong, 1, AsOffset(embedded_image_offset), TiffFileEndianness);
	tiffdir->AddEntry(e6);

	TiffDirEntry e7(TiffTag::SamplesPerPixel, Datatype::Ushort, 1, AsShort(numComponents), TiffFileEndianness);
	tiffdir->AddEntry(e7);

	TiffDirEntry e8(TiffTag::StripByteCounts, Datatype::Ulong, 1, embedded_image_end - embedded_image_offset, TiffFileEndianness);
	tiffdir->AddEntry(e8);

	TiffDirEntry e9(TiffTag::PlanarConfig, Datatype::Ushort, 1, AsShort(1), TiffFileEndianness); // 1 betyr at alle data er i samme plan
	tiffdir->AddEntry(e9);

	TiffDirEntry e10(TiffTag::JPEGTables, Datatype::Xbyte, jpeg_tables_end - jpeg_tables_start, AsOffset(jpeg_tables_start), TiffFileEndianness);
	tiffdir->AddEntry(e10);

	// TIFFTAG YCbCrSubSampling
	int horizontal_divisor = 0;
	int vertical_divisor = 0;

	if (horizontalSampleFactor_Cb == 1 && horizontalSampleFactor_Cr == 1 && (horizontalSampleFactor_Y == 1 || horizontalSampleFactor_Y == 2 || horizontalSampleFactor_Y == 4))
	{
		horizontal_divisor = horizontalSampleFactor_Y;
	}
	if (verticalSampleFactor_Cb == 1 && verticalSampleFactor_Cr == 1 && (verticalSampleFactor_Y == 1 || verticalSampleFactor_Y == 2 || verticalSampleFactor_Y == 4))
	{
		vertical_divisor = verticalSampleFactor_Y;
	}

	if (horizontal_divisor > 0 && vertical_divisor > 0)
	{
		AsShort subsampling_factors(horizontal_divisor, vertical_divisor);
		TiffDirEntry e12(TiffTag::YCbCrSubSampling, Datatype::Ushort, 2, subsampling_factors, TiffFileEndianness);
		tiffdir->AddEntry(e12);
	}
	else if (numComponents > 2)
	{
		// Not used if numComponents == 1
		THROW(L"Illegal subsampling factors!");
	}

	// Insert stuff from Exif here!

	for (auto it = main_dir_entries_from_exif.begin(); it != main_dir_entries_from_exif.end(); ++it)
	{
		tiffdir->AddEntry(*it);
	}

	if (icc_profile_end > icc_profile_begin)
	{
		Offset_t iccprofile_size = icc_profile_end - icc_profile_begin;
		TiffDirEntry e13(TiffTag::IccProfile, Datatype::Xbyte, iccprofile_size, AsOffset(icc_profile_begin), TiffFileEndianness);
		tiffdir->AddEntry(e13);
	}

	if (exifdir_offset > 0)
	{
		TiffDirEntry e14(TiffTag::ExifIFD, Datatype::Ulong, 1, AsOffset(exifdir_offset), TiffFileEndianness);
		tiffdir->AddEntry(e14);
	}

	if (gpsdir_offset > 0)
	{
		TiffDirEntry e15(TiffTag::GPSIFD, Datatype::Ulong, 1, AsOffset(gpsdir_offset), TiffFileEndianness);
		tiffdir->AddEntry(e15);
	}

	tiffdir->SortEntries();

	tiffdir->RebuildBinaryData();
	AddSegmentNopad(TiffFile, S); // This is the end-of-file.  No need for padding at eof.

	// std::wcout << L"______________________________________________\n\n";
	//
	// for (auto p = TiffFile.begin(); p != TiffFile.end(); ++p)
	// {
	//	  (*p)->Dump();
	//	  std::wcout << std::endl;
	// }

	// ____________________________________________________________________________________________________________________________________
	//
	//		WRITE FILE
	// ____________________________________________________________________________________________________________________________________

	FILE* outfile = nullptr;
	errno_t err = _wfopen_s(&outfile, outfilename.c_str(), L"wb");
	if (outfile == nullptr)
	{
		THROW(L"Error opening output file!");
	}
	else
	{
		for (auto p = TiffFile.begin(); p != TiffFile.end(); ++p)
		{
			(*p)->WriteToFile(outfile);
		}
		fclose(outfile);
	}
}

