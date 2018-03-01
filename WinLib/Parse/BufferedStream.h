#if !defined (BUFFEREDSTREAM_H)
#define BUFFEREDSTREAM_H
// -------------------------------
// (c) Reliable Software 2003 - 06
// -------------------------------

class BufferedStream
{
public:
	BufferedStream (unsigned bufSize)
	  : _bufPos (0), 
		_bufEnd (0),
		_bufSize (bufSize),
		_buf (bufSize)
	{
		Assert (bufSize != 0);
	}
	char GetChar ()
	{
		if (_bufPos == _bufEnd)
		{
			FillBuf ();
		}
		Assert (_bufSize > 0);
		Assert (_bufEnd <= _bufSize);
		Assert (_bufPos < _bufEnd);
		return _buf [_bufPos++];
	}
	char LookAhead ()
	{
		if (_bufPos == _bufEnd)
		{
			FillBuf ();
		}
		Assert (_bufSize > 0);
		Assert (_bufEnd <= _bufSize);
		Assert (_bufPos < _bufEnd);
		return _buf [_bufPos];
	}
protected:
	virtual void FillBuf () = 0; // Post-condition: _bufPos < _bufEnd, _bufEnd <= _bufSize
	unsigned BufferSize () const { return _bufSize; }
	char * GetBuf () { return &_buf [0]; }
protected:
	unsigned		  _bufPos;
	unsigned		  _bufEnd;
	unsigned		  _bufSize;
	std::vector<char> _buf;
};

// low level class; just breaks stream into lines, no checking of buffer end
class LineBreaker
{
public:
	LineBreaker (BufferedStream & stream);
	void Advance ();
	std::string const & Get () const { return _line; }
private:
	BufferedStream   & _stream;
	std::string		   _line;
};

class LineSeq
{
public:
	LineSeq () : _done (false) {}
	virtual void Advance () = 0;
	bool AtEnd () const { return _done; }
	std::string const & Get () const { return _line; }
protected:
	std::string		_line;
	bool			_done;
};

#endif
