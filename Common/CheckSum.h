#if !defined (CHECKSUM_H)
#define CHECKSUM_H
//----------------------------------
// (c) Reliable Software 2001 - 2007
//----------------------------------

#include <File/File.h>
class MemFile;

class CheckSum
{
public:
	// default corresponding to empty file
	CheckSum ()
		: _sum (0),
		  _crc (0)
	{}
	CheckSum (unsigned long sum, unsigned long crc)
		: _sum (sum),
		  _crc (crc)
	{}
	CheckSum (char const * buffer, File::Size size);
	CheckSum (MemFile const & file);
	CheckSum (std::string const & filePath);

	void SetWild ()
	{
		_sum = checkSumWildCard;
		_crc = crcWildCard;
	}
	void Init (unsigned long sum, unsigned long crc)
	{
		_sum = sum;
		_crc = crc;
	}
	bool operator == (CheckSum const & cs) const
	{
		return (_sum == cs._sum || _sum == checkSumWildCard || cs._sum == checkSumWildCard)
			&& (_crc == cs._crc || _crc == crcWildCard      || cs._crc == crcWildCard);
	}
	bool operator != (CheckSum const & cs) const
	{
		return (_sum != cs._sum && _sum != checkSumWildCard && cs._sum != checkSumWildCard)
			|| (_crc != cs._crc && _crc != crcWildCard      && cs._crc != crcWildCard);
	}
	bool IsNull () const { return _sum == 0 && _crc == 0; }

	// only for display
	unsigned long GetSum () const { return _sum; }
	unsigned long GetCrc () const { return _crc; }

	static const unsigned long crcWildCard = 0x6789abcd;

private:
	static const unsigned long checkSumWildCard = 0xabcd0000;

private:
	void PutBuf (char const * buffer, unsigned long size);

	unsigned long _sum;
	unsigned long _crc;
};

#endif
