#if !defined (IDEOPTIONS_H)
#define IDEOPTIONS_H
//-----------------------------------------
//  IdeOptions.h
//  (c) Reliable Software, 2000
//-----------------------------------------

class IdeOptions
{
public:
    IdeOptions (unsigned long value);
    void operator= (IdeOptions const & options) 
    { 
        _value = options._value;
    }

	bool IsKeepCheckedOut () const		{ return _bits._keepCheckedOut != 0; }
	bool IsAutoFileType () const		{ return _bits._autoFileType != 0; }
	bool IsTextFile () const			{ return _bits._text != 0; }
	bool IsBinaryFile () const			{ return _bits._binary != 0; }
    bool IsRecursive () const			{ return _bits._deep != 0; }
    bool IsAllInProject () const		{ return _bits._all != 0; }

private:
    union
    {
        unsigned long _value;			// for quick access
        struct
        {
			unsigned long _keepCheckedOut:1;
			unsigned long _autoFileType:1;
			unsigned long _text:1;
			unsigned long _binary:1;
            unsigned long _deep:1;		// Applay command recursively
			unsigned long _all:1;		// Execute command for all files in the project
        } _bits;
    };
};

#endif
