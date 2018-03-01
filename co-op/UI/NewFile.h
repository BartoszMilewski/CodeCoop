#if !defined NEWFILE_H
#define NEWFILE_H
//------------------------------------
//  (c) Reliable Software, 1997 - 2008
//------------------------------------

#include "FileTypes.h"
#include "Resource.h"

#include <Ctrl/Edit.h>
#include <Ctrl/Button.h>
#include <Win/Dialog.h>
#include <Graph/Font.h>
#include <Graph/Canvas.h>

class FileSerializer;

class NewFileData
{
public:
    NewFileData (std::string const & content,
				 std::string const & curFolder,
				 FileType type)
        : _type (type),
		  _define (type.IsHeader ()),
          _newCopyright (false),
		  _default (type.IsHeader () || type.IsSource ()),
          _useContent (type.IsHeader () || type.IsSource ()),
          _content (content),
          _targetFolder (curFolder),
		  _doOpen (false),
		  _doAdd (true)
    {}

    bool IsNameOK ();
	void DisplayNameErrors ();
    void SetName (std::string const & name)
    {
		_name.assign (name);
    }
    void SetCopyright (std::string const & newCopyright);

    void SetType (FileType type) { _type = type; }
    void SetDefault (bool def)   { _default = def; }
    void SetUseContent (bool use){ _useContent = use; }
    void SetDefine (bool define) { _define = define; }
	void SetDoOpen (bool val)	 { _doOpen = val; }
	void SetDoAdd (bool val)	 { _doAdd = val; }

    char const * GetName () const { return _name.c_str (); }
    char const * GetTargetFolder () const { return _targetFolder.c_str (); }
    std::string const & GetContent () const{ return _content; }
    FileType GetType () const         { return _type; }
    bool IsDefault () const           { return _default; }
	bool DoOpen () const			  { return _doOpen; }
	bool DoAdd () const				  { return _doAdd; }
    bool UseDefine () const           { return _define; }
    bool IsNewCopyright () const      { return _newCopyright; }
    bool UseContent () const        { return (_useContent && !_content.empty ()); }
    bool SaveNewDefaultCopyright () const
                                      { return (IsNewCopyright () && IsDefault () && !_useContent); }
	void Serialize (FileSerializer & out) const;

private:
    bool		_newCopyright;
    bool		_define;
    bool		_default;
    bool		_useContent;
	bool		_doOpen; // open file after creating it
	bool		_doAdd; // add to project
    FileType	_type;
    std::string	_content;
    std::string	_name;
	std::string	_targetFolder;
};

class NewFileCtrl : public Dialog::ControlHandler
{
public:
    NewFileCtrl (NewFileData * data);

    bool OnInitDialog () throw (Win::Exception);
    bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();
	bool GetDataFrom (NamedValues const & source);
private:
    Win::Edit			_fname;
    Win::EditReadOnly	_path;
    Win::RadioButton	_header;
    Win::RadioButton	_source;
    Win::RadioButton	_other;
    Win::RadioButton	_binary;
    Win::CheckBox		_define;
    Win::Edit			_copyright;
    Win::CheckBox		_default;
    Win::CheckBox		_useCopyright;
    Win::UpdateCanvas	_copyrightDC;
	Font::Maker			_copyrightFont;
	Font::AutoHandle	_copyrightFace;
	Font::Holder		_copyrightHolder;
    NewFileData *		_dlgData;
};

#endif
