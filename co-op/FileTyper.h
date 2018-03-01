#if !defined FILETYPER_H
#define FILETYPER_H
//----------------------------------
// (c) Reliable Software 1997 - 2008
//----------------------------------

#include "FileTypes.h"
#include "GlobalId.h"
#include "resource.h"

#include <Win/Win.h>
#include <Win/Dialog.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Button.h>
#include <Ctrl/Static.h>
#include <auto_vector.h>

class InputSource;
class UniqueName;
class PathFinder;
class Catalog;
class FileIndex;

class Association
{
public:
	Association (std::string const & extension, FileType type)
        : _extension (extension),
          _type (type)
    {}
	std::string const & GetExtension () const { return _extension; }
    FileType GetType () const { return _type; }

private:
    std::string	_extension;
    FileType	_type;
};

class AssocEntry
{
public:
	char const *	_extension;
	FileType::Enum	_type;
	bool			_addToProject;
};

class AssociationList
{
public:
    AssociationList (Catalog & cat);
	FileType GetFileType (std::string const & ext) const;
	void Add (std::string const & ext, FileType type);

    static FileType     Str2Type (char const * typeName);
private:
    static char const * Type2Str (FileType type);

private:
	Catalog					& _catalog;
    auto_vector<Association>  _list;
    static char				* _fileTypeNames [];
};

class FileTyperData
{
public:
    FileTyperData ()
        : _type (HeaderFile ()),
          _add (true)
    {}

    void SetType (FileType type) { _type = type; }
	void SetFileName (std::string const & fileName);
    void SetAdd (bool add)       { _add = add; }
    FileType GetType () const    { return _type; }
    char const * GetExtension () const { return _ext.c_str (); }
	char const * GetFileName () const { return _fileName.c_str (); }
    bool AddAssociation () const { return _add; }
	bool HasExtension () const { return !_ext.empty (); }
private:
    bool					_add;
    FileType				_type;
	std::string				_fileName;
    std::string				_ext;
};

class FileTypeChangeData
{
public:
	FileTypeChangeData (std::set<std::string> const & extensions,
						GidList const & gids,
						FileIndex const & fileIndex);

	char const * GetExtensions () const { return _allExtensions.c_str (); }
	void SetAdd (bool add)       { _add = add; }
	bool AddAssociation () const { return _add; }
	void SetType (FileType newType);
	FileType GetType () const    { return _type; }
	bool IsTypeSelected () const { return _isTypeSelected; }
private:
	std::string _allExtensions;
	bool		_add;
	bool        _isTypeSelected;
	FileType	_type;
};

class FileTyperCtrl : public Dialog::ControlHandler
{
public:
    FileTyperCtrl (FileTyperData * data)
		: Dialog::ControlHandler (IDD_FILE_TYPE),
		  _dlgData (data)
	{}

    bool OnInitDialog () throw (Win::Exception);
	bool OnApply () throw ();
	bool GetDataFrom (NamedValues const & source);

protected:
	Win::StaticText		_extFrame;
    Win::EditReadOnly   _ext;
    Win::RadioButton    _header;
    Win::RadioButton    _source;
    Win::RadioButton    _text;
    Win::RadioButton    _binary;
    Win::CheckBox       _add;
    FileTyperData *		_dlgData;
};

class FileTypeChangerCtrl : public Dialog::ControlHandler
{
public:
    FileTypeChangerCtrl (FileTypeChangeData * data)
		: Dialog::ControlHandler (IDD_FILE_TYPE_CHANGE),
		  _dlgData (data)
	{}
    bool OnInitDialog () throw (Win::Exception);
	bool OnApply () throw ();
	bool GetDataFrom (NamedValues const & source);
private:
	Win::EditReadOnly   _ext;
    Win::RadioButton    _header;
    Win::RadioButton    _source;
    Win::RadioButton    _text;
    Win::RadioButton    _binary;
    Win::CheckBox       _add;
	FileTypeChangeData * _dlgData;   
};


class Directory;

class FileTyper
{
public:
    FileTyper (Win::Dow::Handle hwnd, Catalog & cat, InputSource * inputSource)
        : _hwnd (hwnd),
          _inputSource (inputSource),
		  _associationDB (cat)
    {}

	FileType GetFileType (std::string const & name);

private:
    AssociationList		_associationDB;
    FileTyperData		_dlgData;
    Win::Dow::Handle	_hwnd;
	InputSource		  * _inputSource;
};

#endif
