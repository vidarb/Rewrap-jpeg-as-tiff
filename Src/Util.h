// File: Util.h
// This file is part of the project: Rewrap-jpeg-as-tiff -- Rewraps a jpeg file into a TIFF container, without loss of information (no re-encoding).
// Copyright(c) 2018 Vidar Bosnes
// This software is licenced under the GNU GENERAL PUBLIC LICENSE Version 3

#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include <stdio.h>
#include <iosfwd>
#include <vector>

typedef std::vector<unsigned char> ByteVector;

enum class Endianness
{
	Big,
	Little
};

typedef long           SLong_t;
typedef unsigned long  ULong_t;
typedef short          Short_t;
typedef unsigned short UShort_t;
typedef char           Byte_t;
typedef unsigned char  UByte_t;
typedef unsigned long  Offset_t;


namespace vibo
{
	class File
	{
		FILE* m_file;
		unsigned long long m_size; // __int64 retval = GetFileSize(f)

	public:
		File(FILE* f);
		operator FILE*()
		{
			return m_file; 
		}
		~File();
		File() = delete;
		File(const File&) = delete;
		File& operator=(const File&) = delete;
	};

	unsigned long long GetFileSize(FILE* f);
	unsigned long long GetFileSize(const std::wstring& filename);
	bool file_exists(const std::wstring& filename);

	int GetByte(FILE* f);

	ByteVector GetBytes(FILE*f, int n);

	ULong_t  GetULong(FILE* f, Endianness e);
	UShort_t GetUShort(FILE* f, Endianness e);

	UByte_t  MakeUByte(const unsigned char* data);
	Byte_t   MakeSByte(const unsigned char* data);
	UShort_t MakeUShort(const unsigned char* data, Endianness e);
	Short_t  MakeSShort(const unsigned char* data, Endianness e);
	ULong_t  MakeULong(const unsigned char* data, Endianness e);
	SLong_t  MakeSLong(const unsigned char* data, Endianness e);

	void binary_copy(unsigned char* destination, const unsigned char* source, int num_elements, int element_size, Endianness e);

	Endianness GetSystemEndianness();

	// Do we need these?
	void PutUShort(unsigned char* memory, const uint16_t& ush, Endianness e);
	void PutUlong(unsigned char* memory, const uint32_t& ulo, Endianness e);

	template<class T> int size(const T& t)
	{
		return static_cast<int>(t.size());
	};
}



#endif