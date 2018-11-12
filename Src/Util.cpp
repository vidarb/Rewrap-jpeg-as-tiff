// File: Util.cpp
// This file is part of the project: Rewrap-jpeg-as-tiff -- Rewraps a jpeg file into a TIFF container, without loss of information (no re-encoding).
// Copyright(c) 2018 Vidar Bosnes
// This software is licenced under the GNU GENERAL PUBLIC LICENSE Version 3

#pragma warning(disable: 4996)
#include "Util.h"
#include <iostream>
#include "Exception.h"
#include <windows.h>
#include <io.h>

namespace vibo
{
	File::File(FILE* f) : m_file(f)
	{
	}

	File::~File()
	{
		if (m_file != nullptr)
		{
			fclose(m_file);
		}
	}


	// ------------------------------------------------------------------------------------------
	//		GetFileSize()
	// ------------------------------------------------------------------------------------------

	unsigned long long GetFileSize(FILE* f)
	{
		if (f == nullptr)
		{
			throw(vibo::Exception(L"GetFileSize: null pointer argument!", __FILE__, __LINE__));
		}
		HANDLE ha = (HANDLE)_get_osfhandle(_fileno(f));
		if (ha == INVALID_HANDLE_VALUE)
		{
			throw(vibo::Exception(L"_get_osfhandle failed!", __FILE__, __LINE__));
		}

		LARGE_INTEGER filesize64;
		BOOL ok = GetFileSizeEx(ha, &filesize64);
		if (!ok)
		{
			throw(vibo::Exception(L"GetFileSizeEx failed!", __FILE__, __LINE__));
		}
		return filesize64.QuadPart;
	}


	unsigned long long GetFileSize(const std::wstring& filename)
	{
		FILE* f = _wfopen(filename.c_str(), L"rb");
		if (f == 0)
		{
			throw(vibo::Exception(L"Error: unable to read first input file!", __FILE__, __LINE__));
		}
		__int64 retval = GetFileSize(f);
		fclose(f);
		return retval;
	}



	// ------------------------------------------------------------------------------------------
	//			file_exists()
	// ------------------------------------------------------------------------------------------

	bool file_exists(const std::wstring& filename)
	{
		FILE* f = _wfopen(filename.c_str(), L"rb");
		if (f == 0)
		{
			return false;
		}
		fclose(f);
		return true;
	}

	// ------------------------------------------------------------------------------------------
	//			Get data from file
	// ------------------------------------------------------------------------------------------

	int GetByte(FILE* f)
	{
		unsigned char buf[2];
		int check = (int)fread(&buf[0], 1, 1, f);
		if (check != 1)
		{
			THROW(L"GetByte: Read error!");
		}
		return buf[0];
	}


	ByteVector GetBytes(FILE*f, int n)
	{
		std::vector<unsigned char> vec(n);
		int check = (int) fread(&vec[0], 1, n, f);
		if (check != n)
		{
			THROW(L"GetBytes: Read error!");
		}
		return vec;
	}

	ULong_t GetULong(FILE* f, Endianness e)
	{
		ByteVector vec(4);
		int check = (int) fread(&vec[0], 1, 4, f);
		if (check != 4)
		{
			THROW(L"GetULong: Read error!");
		}
		if (e == Endianness::Little)
		{
			return vec[0] + 256 * vec[1] + 256 * 256 * vec[2] + 256 * 256 * 256 * vec[3];
		}
		else if (e == Endianness::Big)
		{
			return vec[3] + 256 * vec[2] + 256 * 256 * vec[1] + 256 * 256 * 256 * vec[0];
		}
		ASSERT(false);
		return 0; // never reached
	}

	UShort_t GetUShort(FILE* f, Endianness e)
	{
		ByteVector vec(2);
		int check = (int) fread(&vec[0], 1, 2, f);
		if (check != 2)
		{
			THROW(L"GetUShort: Read error!");
		}
		if (e == Endianness::Little)
		{
			return vec[0] + 256 * vec[1];
		}
		else if (e == Endianness::Big)
		{
			return vec[1] + 256 * vec[0];
		}
		ASSERT(false);
		return 0; // never reached
	}

	UByte_t MakeUByte(const unsigned char* data)
	{
		return data[0];
	}

	Byte_t MakeSByte(const unsigned char* data)
	{
		if (data[0] > 127)
		{
			return -256 + data[0];
		}
		return data[0];
	}

	UShort_t MakeUShort(const unsigned char* data, Endianness e)
	{
		if (e == Endianness::Little)
		{
			return data[0] + 256 * data[1];
		}
		else if (e == Endianness::Big)
		{
			return data[1] + 256 * data[0];
		}
		ASSERT(false);
		return 0; // never reached
	}


	Short_t MakeSShort(const unsigned char* data, Endianness e)
	{
		int retval = MakeUShort(data, e);
		if (retval > 32767)
		{
			return -65536 + retval;
		}
		return retval;
	}


	ULong_t MakeULong(const unsigned char* data, Endianness e)
	{
		static const ULong_t LL_256 = 256;
		if (e == Endianness::Little)
		{
			return data[0] + LL_256 * data[1] + LL_256 * LL_256  * data[2] + LL_256 * LL_256 * LL_256  * data[3];
		}
		else if (e == Endianness::Big)
		{
			return data[3] + LL_256 * data[2] + LL_256 * LL_256  * data[1] + LL_256 * LL_256 * LL_256  * data[0];
		}
		ASSERT(false);
		return 0; // never reached
	}

	SLong_t MakeSLong(const unsigned char* data, Endianness e)
	{
		long long retval = MakeULong(data, e);
		if (retval > 2147483647ll)
		{
			return static_cast<SLong_t>(-2147483648ll + retval);
		}
		return static_cast<SLong_t>(retval);
	}

	Endianness GetSystemEndianness()
	{
		static const int i = 1;
		const char* p = reinterpret_cast<const char*>(&i);
		if (p[0] == 1)
		{
			return Endianness::Little;
		}
		return Endianness::Big;
	}


	void binary_copy(unsigned char* destination, const unsigned char* source, int num_elements, int element_size, Endianness e)
	{
		if (element_size == 1 || e == GetSystemEndianness())
		{
			memcpy(destination, source, num_elements*element_size);
		}
		else
		{
			for (int i = 0; i < num_elements; ++i)
			{
				const unsigned char* src = source + i * element_size;
				unsigned char* dest = destination + i * element_size;
				for (int j = 1; j <= element_size; ++j)
				{
					dest[j - 1] = src[element_size - j]; // NOT TESTED
				}
			}
		}
	}


	// ---- Put Methods
	void PutUShort(unsigned char* memory, const uint16_t& ush, Endianness e)
	{
		const unsigned char* p = reinterpret_cast<const unsigned char*>(&ush);
		if (e == GetSystemEndianness())
		{
			memory[0] = p[0];
			memory[1] = p[1];
		}
		else
		{
			memory[0] = p[1];
			memory[1] = p[0];
		}
	}


	void PutUlong(unsigned char* memory, const uint32_t& ulo, Endianness e)
	{
		const unsigned char* p = reinterpret_cast<const unsigned char*>(&ulo);
		if (e == GetSystemEndianness())
		{
			memory[0] = p[0];
			memory[1] = p[1];
			memory[2] = p[2];
			memory[3] = p[3];
		}
		else
		{
			memory[0] = p[3];
			memory[1] = p[2];
			memory[2] = p[1];
			memory[3] = p[0];
		}
	}
}

