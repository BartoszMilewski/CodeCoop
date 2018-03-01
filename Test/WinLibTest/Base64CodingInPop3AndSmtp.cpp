// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------
#include "precompiled.h"
#include <Parse/BufferedStream.h>
#include <Mail/Smtp.h>
#include <Mail/Pop3.h>
#include <Mail/Base64.h>
#include <File/SerialUnVector.h>

namespace UnitTest
{
	class Base64Output : public GenericOutput
	{
		static const unsigned int LineLength = 76;
		static const unsigned int ByteQuartetsPerLine = LineLength / 4;
	public:
		Base64Output::Base64Output (unsigned int srcSize)
		: _currLineLength (0)
		{
			unsigned int numberOfByteQuartets = srcSize / 3;
			if (srcSize % 3 != 0)
				++numberOfByteQuartets;

			// destination size
			unsigned int destSize = numberOfByteQuartets * 4;
			unsigned int numberOfLines = numberOfByteQuartets / ByteQuartetsPerLine;
			if (numberOfByteQuartets % ByteQuartetsPerLine != 0)
				++numberOfLines;

			// CR + LF will be appended to each line, except for the last one
			destSize += 2 * (numberOfLines - 1);
			
			_dest.reserve (destSize);
		}

		void Put (char c) 
		{
			if (_currLineLength == LineLength)
			{
				_dest.append ("\r\n");
				_currLineLength = 0;
			}
			else
			{
				++_currLineLength;
			}
			_dest.push_back (c);
		}

		char const * GetBuf () const { return &_dest [0]; }
		unsigned int GetSize () const { return _dest.size (); }
	private:
		std::string		_dest;
		unsigned int	_currLineLength;
	};

	// Auxiliary class for testing only: stream a buffer
	class Pop3Base64InputBufferedStream: public BufferedStream
	{
	public:
		Pop3Base64InputBufferedStream (char const * src, unsigned int srcSize)
			: BufferedStream (srcSize)
		{
			_bufPos = 0;
			_bufEnd = srcSize;
			std::copy (&src [0], &src [srcSize], _buf.begin ());
		}
	protected:
		void FillBuf ()	
		{
			_bufPos = 0;
			_bufEnd = 1;
			_buf [0] = '\0';
		}
	};

	void Base64CodingInPop3AndSmtp (unsigned int startLength, unsigned int finishLength)
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

			Base64Output encoded (src.size ());
			Base64::Encode (&src [0], src.size (), encoded);
			
			Pop3Base64InputBufferedStream stream (encoded.GetBuf (), encoded.GetSize ());
			Pop3::Input input (stream);
			Base64::Decoder decoder;
			unmovable_vector<char> dest;
			SerialUnVector<char>::Output output (dest);
			decoder.Decode (input, output);
			if (!std::equal (dest.begin (), dest.end (), src.begin ()))
				throw "The result data does not match the original data.";
		}
	}
}
