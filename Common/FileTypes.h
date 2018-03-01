#if !defined (FILETYPES_H)
#define FILETYPES_H
//
// (c) Reliable Software 1997 -- 2003
//

class FileType
{
	friend class FileData;
	friend class SynchItem;

public:
    FileType ()
		: _value (0)
	{
		_bits._T = typeInvalid;
	}
    FileType (unsigned long value)
        : _value (value)
    {}
    unsigned long GetValue () const { return _value; } 
    char const * GetName () const ;
    void Reset () 
    { 
        _value = 0; 
    }
	void SetUnrecoverable (bool bit) { _bits._U = bit; }

	bool IsEqual (FileType const & type) const
	{
		return _value == type._value;
	}
	bool IsHeader () const
	{
		return _bits._T == typeHeader;
	}
	bool IsSource () const
	{
		return _bits._T == typeSource;
	}
	bool IsText () const
	{
		return _bits._T == typeText || _bits._T == typeWiki;
	}
	bool IsBinary () const
	{
		return _bits._T == typeBinary;
	}
	bool IsFolder () const
	{
		return _bits._T == typeFolder;
	}
	bool IsRoot () const
	{
		return _bits._T == typeRoot;
	}
	bool IsTextual () const
	{
		return IsHeader () || IsSource () || IsText ();
	}
	bool IsRecoverable () const
	{
		return _bits._U == 0;
	}
	bool IsInvalid () const
	{
		return _bits._T == typeInvalid;
	}
	bool IsAuto () const
	{
		return _bits._T == typeAuto;
	}

public:
	enum Enum
	{
		typeHeader = 0,			// Header file
		typeSource = 1,			// Source code file
		typeText = 2,			// Other ASCII file
		typeBinary = 3,			// Non-text file
		typeFolder = 4,			// Folder
		typeRoot = 5,			// Project root folder
		typeInvalid = 6,
		typeAuto = 7,			// Let co-op determine type
		typeWiki = 8
	};

private:
	void Init (long value) { _value = value; }
	bool Verify () const;

private:
    union
    {
        unsigned long _value;       // for quick serialization
        struct
        {
            unsigned long _T:16;	// Type
			unsigned long _U:1;		// Unrecoverable file once present in the project
        } _bits;
    };
};

class HeaderFile : public FileType
{
public:
	HeaderFile ()
		: FileType (typeHeader)
	{}
};

class SourceFile : public FileType
{
public:
	SourceFile ()
		: FileType (typeSource)
	{}
};

class TextFile : public FileType
{
public:
	TextFile ()
		: FileType (typeText)
	{}
};

class BinaryFile : public FileType
{
public:
	BinaryFile ()
		: FileType (typeBinary)
	{}
};

class AutoFile : public FileType
{
public:
	AutoFile ()
		: FileType (typeAuto)
	{}
};

class FolderType : public FileType
{
public:
	FolderType ()
		: FileType (typeFolder)
	{}
};

class RootFolder : public FileType
{
public:
	RootFolder ()
		: FileType (typeRoot)
	{}
};

class InvalidType : public FileType
{
public:
	InvalidType ()
		: FileType (typeInvalid)
	{}
};

#endif
