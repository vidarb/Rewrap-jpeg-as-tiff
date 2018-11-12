// File: ReadJpegMetadata.cpp
// This file is part of the project: Rewrap-jpeg-as-tiff -- Rewraps a jpeg file into a TIFF container, without loss of information (no re-encoding).
// Copyright(c) 2018 Vidar Bosnes
// This software is licenced under the GNU GENERAL PUBLIC LICENSE Version 3

#pragma warning(disable: 4996)

#include "ReadJpegMetadata.h"
#include "JpegSegments.h"
#include "TiffDirEntry.h"
#include "Exception.h"
#include "Util.h"
#include <map>
#include <iostream>

// ----------------------------------------------------------------------------------------------------------------------------------------
//		valid_icc_chunk()
// ----------------------------------------------------------------------------------------------------------------------------------------

bool valid_icc_chunk(const ByteVector& V)
{
	// V starts with FF E2 nn nn, where nn nn is the length of the segment (not counting the initial FF F2)

	int minimum_size = 4 + 12 + 2 + 1; // A profile (or one of severat profile chungs) of 1 byte
	int V_size = vibo::size(V);
	if (V_size < minimum_size)
	{
		return false;
	}
	if (V[0] != 0xff || V[1] != 0xe2)
	{
		THROW(L"Bug: Marker bytes missing!");
	}
	int len = vibo::MakeUShort(&V[2], Endianness::Big); // Jpeg is Big-endian
	ASSERT(2 + len == V_size); // 2+len because the length in the Jpeg segment does not include the initial FF E2

	static ByteVector Signature{'I', 'C', 'C', '_', 'P', 'R', 'O', 'F', 'I', 'L', 'E', '\0'};
	for (int i = 0; i < 12; ++i)
	{
		if (V[i+4] != Signature[i])
		{
			return false;
		}
	}
	return true;
}


// ----------------------------------------------------------------------------------------------------------------------------------------
//		ReadIccProfile()
// ----------------------------------------------------------------------------------------------------------------------------------------
//		A valid ICC_profile chunk has an 18 byte header of the following format: FF EE nn nn ICC_PROFILE 0 X Y,
//		followed by an entire ICC profile, or one of several chunks that together make up the ICC profile.
//		nn nn = size (bigendian) of the segment (minus 2, FF E2 do not count). 
//		0 is the terminator of the ICC_PROFILE string.
//		Y is the number of chunks that make up the profile
//		X is the chunk number (1..Y).
// ----------------------------------------------------------------------------------------------------------------------------------------

ByteVector ReadIccProfile(std::vector<std::shared_ptr<JpegApp2Segment>>& App2Segments)
{
	ByteVector IccProfile{};

	int numchunks = 0;
	std::vector<ByteVector> Chunks;

	for (auto it = App2Segments.begin(); it != App2Segments.end(); ++it)
	{
		const ByteVector& D = (*it)->Data();
		if (valid_icc_chunk(D))
		{
			// valid_icc_chunk guarantees that D.size() >= 18.
			if (numchunks == 0)
			{
				numchunks = D[17];
			}
			else if (numchunks != D[17])
			{
				// All chunks must declare the same number of chunks
				THROW(L"Embedded ICC profile numchunks mismatch!");
			}
			ASSERT(numchunks > 0);
			if (vibo::size(Chunks) == 0)
			{
				Chunks.resize(numchunks);
			}
			int chunkno = D[16];
			if (chunkno < 1 || chunkno > numchunks)
			{
				std::wstring msg{L"ReadIccProfile: Illegal chunk number (number "};
				msg += std::to_wstring(chunkno);
				msg += L" of ";
				msg += std::to_wstring(numchunks);
				msg += L").";
				THROW(msg);
			}
			ASSERT(vibo::size(Chunks[chunkno - 1]) == 0); // chunkno is 1-based, the vector is 0-based
			Chunks[chunkno - 1] = ByteVector(18 + D.begin(), D.end()); // ditto, 18 = 4 {sizeof( ff ee nn nn )} + 12 { sizeof (ICC_PROFILE0) } + 2 { sizeof (chunkno, numchunks) }
		}
	}
	if (numchunks > 0)
	{
		// Concatenate the chunks

		for (int i = 0; i < numchunks; ++i)
		{
			ASSERT(!Chunks[i].empty());
			IccProfile.insert(IccProfile.end(), Chunks[i].begin(), Chunks[i].end());
		}
	}

	/* for debugging
	FILE* dump = _wfopen(L"F:/TIFF-convert/memdump.bin", L"wb");
	if (dump != 0)
	{
		int numbytes = vibo::size(IccProfile);
		int check = (int) fwrite(&IccProfile[0], 1, numbytes, dump);
		fclose(dump);
	}
	*/
	return IccProfile;
}


// ----------------------------------------------------------------------------------------------------------------------------------------
//		valid_exif_chunk()
// ----------------------------------------------------------------------------------------------------------------------------------------

bool valid_exif_chunk(const ByteVector& V)
{
	// V starts with {FF E1 nn nn E X I F 0 0 S1 S2 S3 S4 xx xx xx xx }, where nn nn is the length of the segment, S1 S2 S3 S4 is {I I 2a 0} or {M M 0 2a} and xx xx xx xx is the offset of the TIFF directory

	int minimum_size = 4 + 6 + 4 + 4 + 1; // {FF E1 nn nn E X I F 0 0 S1 S2 S3 S4 xx xx xx xx } plus 1 byte
	int V_size = vibo::size(V);
	if (V_size < minimum_size)
	{
		return false;
	}
	if (V[0] != 0xff || V[1] != 0xe1)
	{
		THROW(L"Bug: Marker bytes missing!");
	}
	int len = vibo::MakeUShort(&V[2], Endianness::Big); // Jpeg is Big-endian
	ASSERT(2 + len == V_size); // 2+len because the length in the Jpeg segment does not include the initial FF E1

	static ByteVector ExifSignature{ 'E', 'x', 'i', 'f', '\0' , '\0' };
	for (int i = 0; i < 6; ++i)
	{
		if (V[i + 4] != ExifSignature[i])
		{
			return false;
		}
	}
	static ByteVector TiffSignature1{ 0x49, 0x49, 0x2a, 0x00 };
	static ByteVector TiffSignature2{ 0x4d, 0x4d, 0x00, 0x2a };

	for (int i = 0; i < 4; ++i)
	{
		if (V[i + 4] != ExifSignature[i])
		{
			return false;
		}
	}

	bool sig1_ok = true;
	for (int i = 0; i < 4; ++i)
	{
		if (V[i + 10] != TiffSignature1[i])
		{
			sig1_ok = false;
		}
	}
	if (sig1_ok)
	{
		return true;
	}

	bool sig2_ok = true;
	for (int i = 0; i < 4; ++i)
	{
		if (V[i + 10] != TiffSignature2[i])
		{
			sig2_ok = false;
		}
	}
	return sig2_ok;
}


// ----------------------------------------------------------------------------------------------------------------------------------------
//		Read a directory corresponding to an offset
// ----------------------------------------------------------------------------------------------------------------------------------------

std::vector<std::tuple<TiffDirEntry, ByteVector>> ReadDirectory(const ByteVector& memory, Offset_t offset, Endianness ee)
{
	std::vector<std::tuple<TiffDirEntry, ByteVector >> directory_info;

	int num_entries = vibo::MakeUShort(&memory[offset], ee);

	Offset_t exifdir_offset = 0;
	Offset_t gpsdir_offset = 0;

	for (int i = 0; i < num_entries; ++i)
	{
		TiffDirEntry e;
		e.InitializeFromMemory(&memory[offset + 2 + 12 * i], ee); //  2 = { num_entries }, 12 = sizeof(dir entry)

		ByteVector V;

		ULong_t datasize = e.GetDataSize();
		if (datasize > 4)
		{
			Offset_t offs = e.GetOffsetField();
			if (offs + datasize < (ULong_t) vibo::size(memory))
			{
				ByteVector tmp(offs + memory.begin(), offs + datasize + memory.begin()); // Correction for endianness is performed in Write_Selected_Entries(), ConvertGraphicsFileToTiff.cpp
				tmp.swap(V);
			}
			else if (offs + datasize == vibo::size(memory))
			{
				// special case, we have just reached the end of mem
				ByteVector tmp(offs + memory.begin(), memory.end());
				tmp.swap(V);
			}
			// else ERROR...
		}
		directory_info.emplace_back(std::make_tuple(e, V));
	}
	return directory_info;
}


// ----------------------------------------------------------------------------------------------------------------------------------------
//		Find the offset to an IFD directory
// ----------------------------------------------------------------------------------------------------------------------------------------

Offset_t find_offset(const std::vector<std::tuple<TiffDirEntry, ByteVector>>& dir, int tagID)
{
	int num_entries = vibo::size(dir);
	for (int i = 0; i < num_entries; ++i)
	{
		const TiffDirEntry& E = std::get<TiffDirEntry>(dir[i]);
		if (E.Tag() == tagID)
		{
			return E.GetOffsetField();
		}
	}
	return 0;
}


// ----------------------------------------------------------------------------------------------------------------------------------------
//		ReadApp1Metadata()
// ----------------------------------------------------------------------------------------------------------------------------------------
//		A valid App1 metadata segment has a 10 byte header of the following format: FF E1 nn nn E X I F 0 0,
//		followed by an TIFF directory header {I, I, 2a, 0 } or {M, M, 0, 2a} followed by a pointer to a tiff directory
//		nn nn = size (bigendian) of the segment (minus 2, FF E1 do not count). 
// ----------------------------------------------------------------------------------------------------------------------------------------

exif_info ReadApp1Metadata(std::vector<std::shared_ptr<JpegApp1Segment>>& App1Segments)
{
	exif_info metadata;

	for (auto it = App1Segments.begin(); it != App1Segments.end(); ++it)
	{
		const ByteVector& D = (*it)->Data();
		if (valid_exif_chunk(D))
		{
			if (D[10] == 0x49)
			{
				metadata.endianness = Endianness::Little;
			}
			else if (D[10] == 0x4D)
			{
				metadata.endianness = Endianness::Big;
			}
			else
			{
				THROW(L"Bug reading App1 metadata!");
			}

			Offset_t dir_offset = vibo::MakeULong(&D[14], metadata.endianness); // The directory offset is located at pos 14, after {FF E1 nn nn E X I F 0 0 S1 S2 S3 S4}
			if (dir_offset + 18 >= (ULong_t) vibo::size(D)) // 18: 2 + 12 + 4: size of directory with one entry
			{
				THROW(L"Invalid directory offset in Exif App1 segment!");
			}

			ByteVector D2(10 + D.begin(), D.end()); // Make a copy such that offsets correspond to vector indices
			metadata.main_dir = ReadDirectory(D2, dir_offset, metadata.endianness);

			Offset_t exifdir_offset = find_offset(metadata.main_dir, TiffTag::ExifIFD);
			if (exifdir_offset != 0)
			{
				metadata.exif_dir = ReadDirectory(D2, exifdir_offset, metadata.endianness);
			}

			Offset_t gpsdir_offset = find_offset(metadata.main_dir, TiffTag::GPSIFD);
			if (gpsdir_offset != 0)
			{
				metadata.gps_dir = ReadDirectory(D2, gpsdir_offset, metadata.endianness);
			}
		}
	}
	return metadata;
}


