// File: Exception.cpp
// This file is part of the project: Rewrap-jpeg-as-tiff -- Rewraps a jpeg file into a TIFF container, without loss of information (no re-encoding).
// Copyright(c) 2018 Vidar Bosnes
// This software is licenced under the GNU GENERAL PUBLIC LICENSE Version 3

// Disable unwanted compiler warnings:
// C4290: C++ Exception Specification ignored
#pragma warning(disable: 4290)
#pragma warning(disable: 4996)

#include "Exception.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <locale>
#include <codecvt>

namespace vibo
{
	namespace // anonymous
	{
		std::wstring mk_what(const std::wstring& msg, const std::string& filename, int lineno)
		{
			std::wstring retval = msg;
			retval += L" File:";
			std::wstring wide_filename = to_wstring(filename);
			retval += wide_filename;
			retval += L" Line:";
			retval += std::to_wstring(lineno);
			retval += L".";
			return retval;
		}

		const char* skip_path(const char* arg)
		{
			const char* retval(0);
			for (const char* p = arg; *p != 0; ++p)
			{
				if (*p == '/' || *p == '\\')
				{
					retval = p+1;
				}
			}
			if (retval == 0 || *retval == '\0')
			{
				retval = arg;
			}
			return retval;
		}
	}


	// —————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
	//		Exception: Exception base class
	// —————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

	Exception::Exception(const wchar_t* msg, const char* fileName, int lineNo) throw()
	:	exception(),
		m_msg(msg),
		m_fileName(skip_path(fileName)),
		m_lineNo(lineNo),
		m_what(),
		m_ansiwhat()
	{
		m_what = mk_what(m_msg, m_fileName, m_lineNo);
		m_ansiwhat = to_string(m_what);
	}


	Exception::Exception(const std::wstring& msg, const char* fileName, int lineNo) throw()
	:	exception(),
		m_msg(msg),
		m_fileName(skip_path(fileName)),
		m_lineNo(lineNo),
		m_what(),
		m_ansiwhat()
	{

		m_what = mk_what(m_msg, m_fileName, m_lineNo);
		m_ansiwhat = to_string(m_what);
	}


	Exception::~Exception() throw()
	{
		m_lineNo = 0;
	}



	Exception::Exception(const Exception& rhs) throw()
	:	exception(rhs),
		m_msg(rhs.m_msg),
		m_fileName(rhs.m_fileName),
		m_lineNo(rhs.m_lineNo),
		m_what(rhs.m_what),
		m_ansiwhat(rhs.m_ansiwhat)
	{
	}


	Exception& Exception::operator=(const Exception& rhs) throw()
	{
		if (&rhs != this)
		{
			static_cast<exception*>(this)->operator=(rhs);
			this->m_fileName = rhs.m_fileName;
			this->m_msg = rhs.m_msg;
			this->m_lineNo = rhs.m_lineNo;
			this->m_what = rhs.m_what;
			this->m_ansiwhat = rhs.m_ansiwhat;
		}
		return *this;
	}


	const char* Exception::what() const throw()
	{
		if (!m_what.empty())
		{
			return m_ansiwhat.c_str();
		}
		return "Undefined exception";
	}


	const wchar_t* Exception::widewhat() const throw()
	{
		if (!m_what.empty())
		{
			return m_what.c_str();
		}
		return L"Undefined exception";
	}


	const wchar_t* Exception::message() const throw()
	{
		if (! m_msg.empty()) return m_msg.c_str();
		return L"Undefined exception";
	}

	// —————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
	//		Warn
	// —————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

	void Warn(const std::wstring& msg)
	{
		HWND hWnd = GetActiveWindow(); // Returns NULL on error, which is OK
		MessageBoxW(hWnd, msg.c_str(), L"", MB_OK | MB_ICONWARNING);
	}

	void Warn(const std::wstring& msg, const char* fileName, int lineNo)
	{
		HWND hWnd = GetActiveWindow(); // Returns NULL on error, which is OK
		std::wstring msg2 = msg;
		msg2 += L"\r\n";
		std::string fn = skip_path(fileName);
		std::wstring fn2(fn.begin(), fn.end());
		msg2 += fn2;
		msg2 += L"  line: ";
		msg2 += std::to_wstring(lineNo);
		MessageBoxW(hWnd, msg2.c_str(), L"", MB_OK | MB_ICONWARNING);
	}

	// —————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
	//		String conversion functions that are needed here are defined here.
	// —————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

	std::string to_string(const std::wstring& wstr)
	{
		using convert_typeX = std::codecvt_utf8<wchar_t>;
		std::wstring_convert<convert_typeX, wchar_t> converterX;
		return converterX.to_bytes(wstr);
	}

	std::wstring to_wstring(const std::string& str)
	{
		using convert_typeX = std::codecvt_utf8<wchar_t>;
		std::wstring_convert<convert_typeX, wchar_t> converterX;

		return converterX.from_bytes(str);
	}


} // end namespace
