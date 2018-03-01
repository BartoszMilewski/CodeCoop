//----------------------------------
// Reliable Software (c) 1998 - 2007
//----------------------------------

#include "precompiled.h"
#include "Crypt.h"
#include "Global.h"

long const maxSeats = 100;

long const mask4  = (1 << 4) - 1;
long const mask5  = (1 << 5) - 1;
long const mask6  = (1 << 6) - 1;
long const mask16 = (1 << 16) - 1;
long const mask18 = (1 << 18) - 1;
long const mask24 = (1 << 24) - 1;
long const mask30 = (1 << 30) - 1;

// 42-bit number
class Num42
{
public:
	Num42 () 
		: _low (0), _high (0) 
	{}

	Num42 (long low, long high)
		: _low (low), _high (high)
	{}

	Num42 (char const * key);
	long GetLow () const { return _low; }
	long GetHigh () const { return _high; }

	void Xor (Num42 & num)
	{
		_low  ^= num._low;
		_high ^= num._high;
	}
	void ShiftAdd (char c)
	{
		_low = (_low << 4) + c;
		long over = _low >> 24;
		_high = (_high << 4) + over;
		_low  &= mask24;
		_high &= mask18;
	}

private:
	// low  24 bits = 6 * 4
	long _low;
	// high 18 bits = 6 * 3
	long _high;
};

// 60-bit number to encode 10 chars of reduced alphabet
class Num60
{
public:
	Num60 () 
		: _low (0), _high (0) 
	{}

	Num60 (long low, long high)
		: _low (low), _high (high)
	{}

	Num60 (char const * key);
	long GetLow () const { return _low; }
	long GetHigh () const { return _high; }

	void Xor (Num60 & num)
	{
		_low  ^= num._low;
		_high ^= num._high;
	}
	void ShiftAdd (char c)
	{
		_low = (_low << 4) + c;
		// 2 bits of carry
		long over = _low >> 30;
		_high = (_high << 4) + over;
		_low  &= mask30;
		_high &= mask30;
	}

private:
	// low  30 bits = 6 * 5
	long _low;
	// high 30 bits = 6 * 5
	long _high;
};

char NumToChar [64] =
{
//   0    1    2    3    4    5    6    7    8    9 
    'c', 'E', 'B', '7', 'e', 'k', 'H', 'n', 'Y', 'p',
//10 0    1    2    3    4    5    6    7    8    9
    '-', 'Q', ':', 'u', 'U', 'r', 'N', 'w', 'R', 'f',
//20 0    1    2    3    4    5    6    7    8    9 
    '@', '9', 'M', 'h', 'K', 'D', '#', '4', '=', 'V',
//30 0    1    2    3    4    5    6    7    8    9 
    'd', 'F', 'C', '2', 'i', 'a', 'S', 'y', '8', 'z',
//40 0    1    2    3    4    5    6    7    8    9 
    'Z', '+', 'o', 'A', 's', '5', 'j', 'J', 'b', 'x',
//50 0    1    2    3    4    5    6    7    8    9 
    'W', 'm', 'q', '6', 'v', 'T', 'G','t', 'L', 'X',
//60 0    1    2    3 
    '3', 'P', 'g', '*'   
};

char num2char (int n)
{
	if (n < 64)
		return NumToChar [n];
	else
		return '\0';
}

char CharToNum [128] =
{
//  00  01  02  03  04  05  06  07  08  09  0a  0b  0c  0d  0e  0f
	00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, // 00
	00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, // 10
//             '#'                          '*' '+'     '-'
	00, 00, 00, 26, 00, 00, 00, 00, 00, 00, 63, 41, 00, 10, 00, 00, // 20
//  0   1   2   3   4   5   6   7   8   9  ':'          '='
	00, 00, 33, 60, 27, 45, 53,  3, 38, 21, 12, 00, 00, 28, 00, 00, // 30
//   @  A   B   C   D   E   F   G   H   I   J   K   L   M   N   O
	20, 43,  2, 32, 25,  1, 31, 56,  6,  0, 47, 24, 58, 22, 16,  0, // 40
//  P   Q   R   S   T   U   V   W   X   Y   Z
	61, 11, 18, 36, 55, 14, 29, 50, 59,  8, 40, 00, 00, 00, 00, 00, // 50
//      a   b   c   d   e   f   g   h   i   j   k   l   m   n   o
	00, 35, 48,  0, 30,  4, 19, 62, 23, 34, 46,  5,  0, 51,  7, 42, // 60
//  p   q   r   s   t   u   v   w   x   y   z
	 9, 52, 15, 44, 57, 13, 54, 17, 49, 37, 39, 00, 00, 00, 00, 00 // 70
};

int char2num (char c)
{
	if (c < 128 && c >= 0)
		return CharToNum [c];
	else
		return 0;
}

Num42::Num42 (char const * key)
	: _low (0), _high (0) 
{
	// high = key [0] key [1] key [2]
	// low  = key [3] key [4] key [5] key [6]
  	int i;
	for (i = 0; i < 3; i++)
	{
		_high = (_high << 6) | char2num (key [i]);
	}
	for ( ; i < 7; i++)
	{
		_low = (_low << 6) | char2num (key [i]);
	}
}

Num60::Num60 (char const * key)
	: _low (0), _high (0) 
{
	// high = key [0] key [1] key [2] key [3] key [4]
	// low  = key [5] key [6] key [7] key [8] key [9]
  	int i;
	for (i = 0; i < 5; i++)
	{
		_high = (_high << 6) | char2num (key [i]);
	}
	for ( ; i < 10; i++)
	{
		_low = (_low << 6) | char2num (key [i]);
	}
}

bool Decode10 (std::string name, std::string key, int & version, int & seats, char & product);

bool IsCurrentVersion (int version)
{
	return version == TheCurrentMajorVersion;
}

// Must be better or equal to current product
bool IsValidProduct (char productId)
{
	switch (productId)
	{
	case coopProId:
		return true;
	case coopLiteId:
		return TheCurrentProductId != coopProId;
	case clubWikiId:
		return TheCurrentProductId != coopProId && TheCurrentProductId != coopLiteId;
	}
	return false;
}

// licenseString = concatenation of licensee name and license key
// Returns false if invalid license. Sets version number
bool DecodeLicense (std::string const & licenseString, int & version, int & seats, char & product)
{
	seats = 0;
	version = 0;

	Assert (licenseString.length () != 0);
	std::string::size_type markerPos = licenseString.find ('\n');
	Assert (markerPos != std::string::npos);
	int keyLen = licenseString.length () - markerPos - 1;
	if (keyLen == 10)
	{
		return Decode10 (licenseString.substr (0, markerPos), 
						 licenseString.substr (markerPos + 1), 
						 version, 
						 seats,
						 product);
	}
	else
		return false;
}

bool Decode10 (std::string name, std::string key, int & version, int & seats, char & product)
{
	Num60 num (key.c_str ());
	Num60 hash;
	for (unsigned i = 0; i < name.length (); i++)
		hash.ShiftAdd (name [i]);
	num.Xor (hash);
	long low = num.GetLow ();
	version = low & mask5;
	low >>= 5;
	// Check the product name
	// Last letter is the product type (P=professional, L=lite, W=wiki)
	product = static_cast<char>(low & mask5) + 'A';
	low >>= 5;
	// decode the remaining 3 letters
	for (int j = 2; j >= 0; j--)
	{
		if ("COO*" [j] - 'A' != (low & mask5))
			return false;
		low >>= 5;
	}
	// redundancy check
	if ((low & mask5) != version)
		return false;

	bool isValid = true;
	// 4 bits for major version number
	version = low;
	if (version < 4) // 10-char keys started with v. 4
		isValid = false;

	long high = num.GetHigh ();
	seats = (high & 0xffff);
	// redundancy check
	if (seats != (high >> 16))
	{
		seats = 0;
	}
	if (seats == 0)
		isValid = false;
	return isValid;
};

// return 10-character key
std::string EncodeLicense10 (std::string const & licensee, int seats, char product)
{
	char key [11];
	if (seats >= maxSeats)
	{
		// no more than 64k seats
		key [0] = '\0';
		return std::string ();
	}
	// encode seats redundantly
	long high = (seats << 16)| seats;
	long low = TheCurrentMajorVersion;
	// Product name
	for (int j = 0; j < 3; j++)
	{
		low <<= 5;
		low |= "COO*" [j] - 'A';
	}
	// Last letter identifies product
	low <<= 5;
	low |= product - 'A';

	low = (low << 5) + TheCurrentMajorVersion;

	// high contains # of seats and minor version
	// low contains major version and product string
	Num60 num (low, high);
	// Hash licensee name
	Num60 hash;
	int nameLen = licensee.size ();
	for (int i = 0; i < nameLen; i++)
		hash.ShiftAdd (licensee [i]);
	// xor hash into our number
	num.Xor (hash);

	// Turn it into a string
	// high = key [0] key [1] key [2] key [3] key [4]
	// low  = key [5] key [6] key [7] key [8] key [9]
	high = num.GetHigh ();
	low = num.GetLow ();

	key [4] = num2char (high & mask6);
	high >>= 6;
	key [3] = num2char (high & mask6);
	high >>= 6;
	key [2] = num2char (high & mask6);
	high >>= 6;
	key [1] = num2char (high & mask6);
	high >>= 6;
	key [0] = num2char (high & mask6);

	key [9] = num2char (low & mask6);
	low >>= 6;
	key [8] = num2char (low & mask6);
	low >>= 6;
	key [7] = num2char (low & mask6);
	low >>= 6;
	key [6] = num2char (low & mask6);
	low >>= 6;
	key [5] = num2char (low & mask6);
	key [10] = '\0';
	return std::string (key);
}

