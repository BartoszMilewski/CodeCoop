//----------------------------------
// (c) Reliable Software 2001 - 2006
//----------------------------------

#include <WinLibBase.h>
#include "StringOp.h"

#include <iomanip>

UpCaseTable const TheUpCaseTable;

// returns offset of the first match or npos
unsigned NocaseContains (std::string const & s1, std::string const & s2)
{
	unsigned size2 = s2.size ();
	if (s1.size () < size2)
		return std::string::npos;
	unsigned maxOff = s1.size () - size2;
	for (unsigned off = 0; off <= maxOff; ++off)
	{
		if (std::equal (s1.begin () + off, 
						s1.begin () + off + size2,
						s2.begin (), 
						NocaseEqualChar (TheUpCaseTable)))
		{
			return off;
		}
	}
	return std::string::npos;
}

// return the position of the segment of s2 starting at off2, lenght len 
// inside the segment of s1 starting at off1
unsigned NocaseFind (std::string const & s1, unsigned off1, std::string const & s2, unsigned off2, unsigned len)
{
	unsigned size1 = s1.size () - off1;
	if (size1 < len)
		return std::string::npos;
	unsigned maxOff = s1.size () - len;
	for (unsigned off = off1; off <= maxOff; ++off)
	{
		if (std::equal (s1.begin () + off, 
						s1.begin () + off + len,
						s2.begin () + off2, 
						NocaseEqualChar (TheUpCaseTable)))
		{
			return off;
		}
	}
	return std::string::npos;
}

MultiString::const_iterator MultiString::const_iterator::operator ++()
{
	while (*_p != '\0')
		++_p;
	++_p;
	return _p;
}

MultiString::MultiString ()
{
	_str += '\0';
}

MultiString::MultiString (std::string const & str)
	: _str (str)
{
	Validate ();
}

bool MultiString::empty () const
{
	// Empty multi-string contains only two '\0' chars
	return _str.length () == 1;
}

void MultiString::push_back (std::string const & str)
{
	if (str.empty ())
		throw Win::InternalException ("Internal error: empty string added to the multi-string.");

	if (empty ())
		_str.clear ();

	_str += str;
	_str += '\0';
}

MultiString::const_iterator MultiString::end () const
{
	if (empty ())
		return _str.begin ();

	return _str.end ();
}

void MultiString::Assign (char const * chars, unsigned charsLen)
{
	_str.assign (chars, charsLen);
	Validate ();
}

void MultiString::Validate ()
{
	unsigned int len = _str.length ();
	if (len < 1 || _str [len - 1] != '\0')
		throw Win::InternalException ("Internal error: illegal multi-string.");
}

int ToInt (std::string const & str)
{
	return ::atoi (str.c_str ());
}

// Warning: will it work for 64-bits?
HWND ToHwnd(std::string const & str)
{
	Assert(sizeof(HWND) <= sizeof(int));
	return reinterpret_cast<HWND>(::atoi(str.c_str()));
}

bool HexStrToUnsigned (char const * str, unsigned long & result)
{
	char * p;
	result = ::strtoul (str, &p, 16);
	return p - str == ::strlen (str);
}

bool StrToUnsigned (char const * str, unsigned long & result)
{
	char * p;
	result = ::strtoul (str, &p, 10);
	return p - str == ::strlen (str);
}

std::string ToHexString (unsigned value)
{
    std::ostringstream buffer;
	buffer << std::hex << value;
    return buffer.str ();
}

std::string ToHexStr (char c)
{
	static const char HexDigits [] = "0123456789abcdef";
	unsigned char highBitQuartet = static_cast<unsigned char> (c) >> 4;
	unsigned char lowBitQuartet  = static_cast<unsigned char> (c) & 0x0f;

	std::string hexStr;
	hexStr += HexDigits [highBitQuartet];
	hexStr += HexDigits [lowBitQuartet];

	return hexStr;
}

std::wstring ToWString (std::string const & src)
{
	std::wstring wStr;
	int translatedLength = ::MultiByteToWideChar (CP_ACP, 0, src.c_str (), -1, 0, 0);
	if (translatedLength == 0)
		throw Win::Exception ("Internal error: Cannot get Unicode string length", src.c_str ());

	wStr.reserve (translatedLength);
	if (::MultiByteToWideChar (CP_ACP, 0, src.c_str (), -1, 
		writable_wstring (wStr), translatedLength) == 0)
	{
		throw Win::Exception ("Internal error: Cannot convert ANSI string to Unicode", src.c_str ());
	}

	return wStr;
}

std::string ToMBString (wchar_t const * wstr)
{
	std::string str;
	int translatedLength = ::WideCharToMultiByte (CP_ACP, 0, wstr, -1, 0, 0, 0, 0);
	if (translatedLength == 0)
		throw Win::Exception ("Internal error: Cannot get multibyte string length");

	str.reserve (translatedLength);
	if (::WideCharToMultiByte (CP_ACP, 0, wstr, -1, 
		writable_string (str), translatedLength, 0, 0) == 0)
	{
		throw Win::Exception ("Internal error: Cannot convert Unicode string to ANSI");
	}
	return str;
}

std::string FormatFileSize (long long size)
{
	if (size == 0)
		return "0 KB";

	long long sizeInKb;
	if (size > 1023)
	{
		sizeInKb = size >> 10;
		if (size % 1024 > 512)
			sizeInKb += 1;
	}
	else
	{
		return "1 KB";
	}

	std::vector<std::string> thousandGroups;
	while (sizeInKb != 0)
	{
		long long group = sizeInKb % 1000;
		sizeInKb /= 1000;
		if (sizeInKb != 0)
		{
			std::ostringstream groupStr;
			groupStr << std::setw (3) << std::setfill ('0') << group;
			thousandGroups.push_back (groupStr.str ());
		}
		else
		{
			// Last group
			thousandGroups.push_back (ToString (group));
		}
	}
	
	std::string sizeStr;
	while (!thousandGroups.empty ())
	{
		sizeStr += thousandGroups.back ();
		sizeStr += " ";
		thousandGroups.pop_back ();
	}
	sizeStr += "KB";
	return sizeStr;
}

bool HasGuidFormat (std::string const & str)
{
	// {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}
	// X-hex digit: 0..9, A..F
	if (str.length () != 38)
		return false;

	if (str [0] != '{' || str [37] != '}')
		return false;

	for (int i = 1; i < 37; ++i)
	{
		if (i == 9 || i == 14 || i == 19 || i == 24)
		{
			if (str [i] != '-')
				return false;
		}
		else if (!std::isxdigit (str [i]))
			return false;
	}
	return true;
}

void TrimmedString::Init (std::string const & str)
{
	// Eat leading white spaces
	size_t nonSpace = str.find_first_not_of (" \t");
	if (nonSpace != std::string::npos)
		assign (str.substr (nonSpace));
	// Eat trailing white spaces
	nonSpace = find_last_not_of (" \t");
	if (nonSpace != std::string::npos)
		erase (nonSpace + 1);
}

std::string NamedValues::GetValue (std::string const & name) const
{
	Iterator it = _map.find (name);
	if (it != _map.end ())
		return it->second;
	else
		return std::string ();
}

#if !defined (NDEBUG) && !defined (BETA)

// MultiString unit test
namespace UnitTest
{
	void Compare (std::vector<std::string> const & testStrings, MultiString const & mString)
	{
		std::string expectedResult;
		for (std::vector<std::string>::const_iterator iter = testStrings.begin (); iter != testStrings.end (); ++iter)
		{
			expectedResult += *iter;
			expectedResult += '\0';
		}
		std::string const & multiString = mString.str ();
		Assert (multiString.length () == expectedResult.length ());
		Assert (multiString == expectedResult);
		std::vector<std::string>::const_iterator testStr = testStrings.begin ();
		MultiString::const_iterator multiStringElement = mString.begin ();
		for (; testStr != testStrings.end () && multiStringElement != mString.end ();
			++testStr, ++multiStringElement)
		{
			Assert (*testStr == *multiStringElement);
		}
		Assert (multiStringElement == mString.end ());
		Assert (testStr == testStrings.end ());
	}

	void Test (std::vector<std::string> const & testStrings)
	{
		MultiString mString;
		for (std::vector<std::string>::const_iterator iter = testStrings.begin (); iter != testStrings.end (); ++iter)
		{
			mString.push_back (*iter);
		}
		Compare (testStrings, mString);
	}

	void MultiStringTest (std::ostream & out)
	{
		out << "Multi string test" << std::endl;
		{
			// Empty multi string
			MultiString mString;
			Assert (mString.empty ());

			for (MultiString::const_iterator iter = mString.begin (); iter != mString.end (); ++iter)
			{
				throw Win::InternalException ("MultiString unit test failure: iteration over empty multi string (1)");
			}

			std::string const & multiString = mString.str ();
			std::string expectedResult;
			expectedResult += '\0';

			Assert (multiString.length () == 1);
			Assert (multiString == expectedResult);

			for (MultiString::const_iterator iter = mString.begin (); iter != mString.end (); ++iter)
			{
				throw Win::InternalException ("MultiString unit test failure: iteration over empty multi string (2)");
			}
		}
		{
			std::vector<std::string> testString;

			// Adding C-string to the multi string
			testString.push_back ("abc");
			Test (testString);

			// Adding multiple C-strings to the multi string
			testString.push_back ("xyz.");
			Test (testString);
		}
		{
			std::string expectedResult ("abc");
			expectedResult += '\0';
			expectedResult += "xyz.";
			expectedResult += '\0';

			// Constructing multi string
			{
				MultiString mString (expectedResult);

				Assert (mString.str () == expectedResult);
			}

			// Assigning multi string
			{
				MultiString mString;
				mString.Assign (expectedResult.c_str (), expectedResult.length ());

				Assert (mString.str () == expectedResult);
			}
		}
		{
			// Copying standard container to the multi string
			std::vector<std::string> testString;
			testString.push_back ("abc");
			testString.push_back ("xyz.");
			MultiString mString;
			std::copy (testString.begin (), testString.end (), std::back_inserter (mString));

			Compare (testString, mString);
		}
		{
			// Copying multi string to the standard container
			std::vector<std::string> testString;
			MultiString mString;
			mString.push_back ("abc");
			mString.push_back ("xyz.");
			std::copy (mString.begin (), mString.end (), std::back_inserter (testString));

			Compare (testString, mString);
		}

		if (0)
		{
			// Validating multi string
			try
			{
				MultiString mString;
				mString.push_back ("");
				Assert (!"MultiString unit test failure: validation (1)");
			}
			catch (Win::InternalException)
			{
			}

			try
			{
				std::string str;
				MultiString mString (str);
				Assert (!"MultiString unit test failure: validation (2)");
			}
			catch (Win::InternalException)
			{
			}

			try
			{
				std::string str ("abc");
				MultiString mString;
				mString.Assign (str.c_str (), str.length ());
				Assert (!"MultiString unit test failure: validation (3)");
			}
			catch (Win::InternalException)
			{
			}
		}
		out << "Passed!" << std::endl;
	}
}

#endif
