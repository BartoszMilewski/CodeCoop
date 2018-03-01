#if !defined (BASE64_H)
#define BASE64_H
// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------

#include <GenericIo.h>
#include <bitset>
#include <unmovable_vector.h>

// Based on RFC #3548

namespace Base64
{
	class Exception : public Win::InternalException
	{
	public:
		Exception (char const * msg)
			: Win::InternalException (msg)
		{}
	};

	std::string Encode (std::string const & src);
	void Encode (char const * buf, unsigned int bufSize, GenericOutput & output);

	class Decoder
	{
	public:
		Decoder ();

		template <class Output>
		void Decode (GenericInput<'\0'> & input, Output & output);
	private:
		unsigned char	 DecodingTable [256];
		std::bitset<256> _isLetterLookup;
	};

	// For testing
	class SimpleInput : public GenericInput<'\0'>
	{
	public:
		SimpleInput (std::string const & src)
		: _it (src.begin ()), _end (src.end ())
		{
			if (src.size () < 4 ||	src.size () % 4 != 0)
				throw Base64::Exception ("The given base64 encoded data is corrupted.");
		}
		char Get ()
		{
			return _it == _end ? '\0' : *_it++;
		}
	private:
		std::string::const_iterator		_it;
		std::string::const_iterator		_end;
	};
}

#endif
