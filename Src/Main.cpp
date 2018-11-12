// File: Main.cpp
// This file is part of the project: Rewrap-jpeg-as-tiff -- Rewraps a jpeg file into a TIFF container, without loss of information (no re-encoding).
// Copyright(c) 2018 Vidar Bosnes
// This software is licenced under the GNU GENERAL PUBLIC LICENSE Version 3

// Command arguments $(TargetDir)\Testfile.JPG
// Working directory  $(ProjectDir)

#pragma warning(disable: 4996)
#include <iostream>
#include <string>
#include <vector>
#include "Exception.h"
#include "Util.h"
#include "GraphicsFile.h"
#include "TiffDirEntry.h"
#include "TiffSegments.h"
#include "JpegSegments.h"
#include "CreateSegment.h"

#include "ConvertJpegToTiff.h"



void ReadFile(std::wstring fn, GraphicsVector& G);


int wmain(int argc, wchar_t* argv[])
{
	try
	{
		GraphicsVector G;
		if (argc > 1)
		{
			std::wstring infile_name = argv[1];
			std::wstring outfile_name{};
			if (argc > 2)
			{
				std::wstring outfile_name = argv[2];
			}
			else
			{
				auto pos = infile_name.find_last_of(L'.'); // Find extension
				if (pos != std::wstring::npos)
				{
					outfile_name = infile_name.substr(0, pos);
				}
				else
				{
					outfile_name = L"JPEG-COMPRESSED-TIFF-FILE";
				}
				outfile_name += L".tif";
			}

			if (vibo::file_exists(outfile_name))
			{
				std::wcerr << std::endl;
				std::wcerr << L"Warning: " << '"' << outfile_name << '"' << L" exists!" << std::endl;
				outfile_name = L"JPEG-COMPRESSED-TIFF-FILE.tif";
				std::wcerr << L"Writing to " << '"' << outfile_name << '"' << L" instead!" << std::endl << std::endl;
			}
			std::wcerr << L"Infile:  " << infile_name << std::endl;
			std::wcerr << L"Outfile: " << outfile_name << std::endl;

			ReadFile(infile_name, G);
			// std::wcout << L"\n\n";
			// Dump(G);
			ConvertJpegToTiff(G, outfile_name);
		}
	}
	catch (std::wstring& e)
	{
		std::wcerr << L"Exception: " << e << L"\n";
		std::wcerr << L"Terminating!\n";
	}
	catch(vibo::Exception& e)
	{
		std::wcerr << L"Exception: " << e.widewhat() << L"\n";
		std::wcerr << L"Terminating!\n";
	}
	return 0;
}


void ReadFile(std::wstring fn, GraphicsVector& G)
{
	vibo::File f(_wfopen(fn.c_str(), L"rb"));
	if (f == nullptr)
	{
		std::wcerr << L"Error opening file \'" << fn << "\'\n";
		exit(0);
	}

	ByteVector vec = vibo::GetBytes(f, 4);
	Filetype ft = Filetype::Unknown;
	if (vec == ByteVector{0x49, 0x49, 0x2a, 00})
	{
		ft = Filetype::TIFF_Little_endian;
	}
	else if (vec == ByteVector{0x4d, 0x4d, 00, 0x2a})
	{
		ft = Filetype::TIFF_Big_endian;
	}
	else if (vec == ByteVector{ 0xff, 0xd8, 0xff, 0xe0 })
	{
		ft = Filetype::JPEG; // JPEG-Jfif
	}
	else if (vec == ByteVector{ 0xff, 0xd8, 0xff, 0xe1 })
	{
		ft = Filetype::JPEG; // JPEG-Exif
	}
	else
	{
		std::wcout << L"Not a tiff or jpeg file\n";
		exit(0);
	}

	if (ft == Filetype::TIFF_Big_endian || ft == Filetype::TIFF_Little_endian)
	{
		Offset_t first_directory_offset = ReadTiffHeader(f, ft, G, 0);
		ReadTiffDirectories(f, ft, G, first_directory_offset);
	}
	else if (ft == Filetype::JPEG)
	{
		Offset_t filesize = static_cast<Offset_t> (vibo::GetFileSize(f));
		ReadJpegFileOrEmbeddedSection(f, G, 0, filesize, L"JPEG file");
	}
}

