// File: Exception.h
// This file is part of the project: Rewrap-jpeg-as-tiff -- Rewraps a jpeg file into a TIFF container, without loss of information (no re-encoding).
// Copyright(c) 2018 Vidar Bosnes
// This software is licenced under the GNU GENERAL PUBLIC LICENSE Version 3

#ifndef VIBO_EXCEPTION_H_INCLUDED
#define VIBO_EXCEPTION_H_INCLUDED

#include <string>

namespace vibo
{

	class Exception : public std::exception
	{
		std::wstring		m_msg;
		std::string		    m_fileName;
		int   m_lineNo;
		std::wstring		m_what;
		std::string         m_ansiwhat;

	public:
		Exception(const wchar_t* msg, const char* fileName, int lineNo) throw();
		Exception(const std::wstring& msg, const char* fileName, int lineNo) throw();
		virtual ~Exception() throw();
		Exception(const Exception& rhs) throw();
		Exception& operator=(const Exception& rhs) throw();
		virtual const char* what() const throw();
		virtual const wchar_t *widewhat() const throw();
		virtual const wchar_t* message() const throw();
	};

	void Warn(const std::wstring& msg);
	void Warn(const std::wstring& msg, const char* fileName, int lineNo);

	// Two string conversion functions needed by Exception are defined here!

	std::string to_string(const std::wstring& wstr);
	std::wstring to_wstring(const std::string& str);
}

#define ASSERT(condition) if (!(condition)) {throw(vibo::Exception(L"Assert failure", __FILE__, __LINE__));}
#define THROW(message)     throw(vibo::Exception(message, __FILE__, __LINE__))

#endif

