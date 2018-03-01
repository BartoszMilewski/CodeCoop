// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------
#include <WinLibBase.h>
#include "Base64.h"
#include "GenericIo.h"

namespace Base64
{
	const unsigned char CodingTable [65] = 
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";
};

Base64::Decoder::Decoder ()
{
	Assert (sizeof (Base64::CodingTable) == 65 * sizeof (char));

	for (unsigned char i = 0; i < 64; ++i)
	{
		_isLetterLookup [Base64::CodingTable [i]] = true;
	}

	std::fill_n (DecodingTable, 256, 255);
	for (unsigned char i = 0; i < 64; ++i)
	{
		DecodingTable [Base64::CodingTable [i]] = i;
	}
}

// ==========================================================
class StringOutput : public GenericOutput, public std::string
{
public:
	StringOutput (unsigned int srcSize)
	{
		Assert (srcSize > 0);
		unsigned int srcSize4 = srcSize * 4;
		unsigned int destSize = srcSize4 / 3 + srcSize4 % 3;
		reserve (destSize); 
	}
	void Put (char c) { push_back (c); }
};

std::string Base64::Encode (std::string const & src)
{
	Assert (!src.empty ());
	StringOutput dest (src.size ());
	Base64::Encode (src.c_str (), src.size (), dest);
	return dest;
}

void Base64::Encode (char const * src, unsigned int srcSize, GenericOutput & output)
{
	Assert (srcSize > 0);

	unsigned int const numberOfDataBytesInLastTrio = srcSize % 3;
	unsigned int const numberOfBytesInFullTrios = srcSize - numberOfDataBytesInLastTrio;

	for (unsigned int i = 0; i < numberOfBytesInFullTrios; )
	{
		unsigned int trio = 0;
		for (unsigned int n = 0; n < 3; ++n)
		{
			trio |= static_cast<unsigned char> (src [i]);
			++i;
			trio <<= 8;
		}
		for (unsigned int k = 0; k < 4; ++k)
		{
			unsigned char idx = (unsigned char)(trio >> 26);
			output.Put (static_cast<char>(Base64::CodingTable [idx]));
			trio <<= 6;
		}
	}
	
	if (numberOfDataBytesInLastTrio != 0)
	{
		// data bytes of the last source trio
		unsigned int trio = 0;
		for (unsigned int i = numberOfBytesInFullTrios; i < srcSize; ++i)
		{
			trio |= static_cast<unsigned char> (src [i]);
			trio <<= 8;
		}
		trio <<= 8 * (3 - numberOfDataBytesInLastTrio);
		for (unsigned int k = 0; k < numberOfDataBytesInLastTrio + 1; ++k)
		{
			unsigned char idx = (unsigned char)(trio >> 26);
			output.Put (static_cast<char>(Base64::CodingTable [idx]));
			trio <<= 6;
		}
		// padding
		for (unsigned int k = 0; k < 3 - numberOfDataBytesInLastTrio; ++k)
		{
			output.Put ('=');
		}
	}

	// inform output that the job is done
	output.Flush ();
}

template<class Output>
void Base64::Decoder::Decode (GenericInput<'\0'> & input, Output & output)
{
	for (;;)
	{
		// four 6-bit values end-to-end
		unsigned int quartet = 0;
		unsigned int shift = 24;
		int countEq = 0; // count trailing equal signs
		for (unsigned int i = 0; i < 4; ++i)
		{
			char c = input.Get ();

			if (c == '\0' && i == 0)
				return;
			
			if (c == '=')
			{
				if (i < 2)
				{
					throw Base64::Exception ("The base64 encoded input is corrupted.");
				}
				else if (i == 2)
				{
					c = input.Get ();
					if (c != '=')
						throw Base64::Exception ("The base64 encoded input is corrupted.");
				}
				countEq = 4 - i;
				break;
			}
			else if (_isLetterLookup.test (static_cast<unsigned char>(c)))
			{
				shift -= 6;
				quartet |= (DecodingTable [static_cast<unsigned char>(c)] << shift);
			}
			else
				throw Base64::Exception ("The base64 encoded input is corrupted."); 
		}
		// split it into three 8-bit values
		char b = (quartet >> 16) & 0xff;
		output.Put (b);
		if (countEq == 2)
			break;
		b = (quartet >> 8) & 0xff;
		output.Put (b);
		if (countEq == 1)
			break;
		b = quartet & 0xff;
		output.Put (b);
	}
}

// ===============
#include <File/SerialUnVector.h>
namespace UnitTest
{
	void Base64Coding (unsigned int startLength, unsigned int finishLength)
	{
		for (unsigned int i = startLength; i < finishLength; ++i)
		{
			// generate random string of length of i.
			std::string src;
			src.resize (i);
			for (unsigned int j = 0; j < i; ++j)
			{
				src [j] = std::rand () % 256;
			}
			std::string encoded = Base64::Encode (src);
			
			Base64::SimpleInput input (encoded);
			Base64::Decoder decoder;
			unmovable_vector<char> dest;
			SerialUnVector<char>::Output output (dest);
			decoder.Decode (input, output);
			
			if (!std::equal (dest.begin (), dest.end (), src.begin ()))
				throw Base64::Exception ("The result data does not match the original data.");
		}
	}
}
