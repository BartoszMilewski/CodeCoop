#if !defined (BITCODE_H)
#define BITCODE_H
//-----------------------------------------------------
//  BitCode.h
//  (c) Reliable Software 2001
//-----------------------------------------------------

#include <Dbg/Assert.h>

class BitCode
{
public:
	BitCode ()
		: _code (0),
		  _len (0)
	{}
	BitCode (unsigned int code, unsigned int len)
		: _code (code),
		  _len (len)
	{}
	BitCode (BitCode const & code)
		: _code (code._code),
		  _len (code._len)
	{}

	unsigned long long GetCode () const { return _code; }
	unsigned int GetLen () const { return _len; }

	void AppendBits (unsigned int bits, unsigned int len)
	{
		Assert (_len + len < std::numeric_limits<unsigned int>::digits);
		_code <<= len;
		_code |= bits;
		_len += len;
	}

private:
	unsigned int	_code;
	unsigned int	_len;
};

#endif
