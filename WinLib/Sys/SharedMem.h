#if !defined (SHAREDMEM_H)
#define SHAREDMEM_H
//----------------------------------------------------
// SharedMem.h
// (c) Reliable Software 2000, 01
//
//----------------------------------------------------

class SharedMem
{
public:
	SharedMem ()
		: _buf (0),
		  _size (0),
		  _handle (0)
	{}
	SharedMem (unsigned int size)
	{
		Create (size);
	}
	SharedMem (std::string const & name, unsigned int size)
	{
		Open (name, size);
	}
	~SharedMem () { Close (); }

	void Create (unsigned int size, std::string const & name = std::string (), bool quiet = false);
	void Open (std::string const & name, unsigned int size, bool quiet = false);

	unsigned int GetHandle () const { return reinterpret_cast<unsigned int>(_handle); }
	std::string const & GetName () const { return _name; }
	unsigned int GetSize () const { return _size; }
    char & operator [] (int i) { return _buf [i]; }
    char const & operator [] (int i) const { return _buf [i]; }
	unsigned char * GetRawBuf () { return reinterpret_cast<unsigned char *>(_buf); }
	bool IsOk () const { return _handle != 0; }

	void Map (unsigned int handle);

private:
	void Map (bool quiet = false);
	void Close ();

protected:
	char *			_buf;
	unsigned int	_size;

private:
	std::string		_name;
	HANDLE			_handle;
};

#endif
