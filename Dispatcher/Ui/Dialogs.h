#if !defined (DIALOGS_H)
#define DIALOGS_H
//----------------------------------
// (c) Reliable Software 1998 - 2008
// ---------------------------------

#include <Win/Dialog.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Static.h>
#include <Ctrl/Button.h>
#include <Ctrl/ListView.h>

class ScriptTicket;

class DeleteOrIgnoreCtrl : public Dialog::ControlHandler
{
public:
    DeleteOrIgnoreCtrl (ScriptTicket const & script);

	bool OnInitDialog () throw (Win::Exception);
    bool OnApply () throw ();

private:
	Win::StaticText	_scriptInfoFrame;
    Win::Edit		_scriptName;
    Win::Edit		_project;
    Win::Edit		_senderHubId;
    Win::Edit		_senderId;
	Win::Edit		_comment;

	Win::ReportListing	_recipients;
    ScriptTicket const &	_script;
};

class BadScriptData
{
    friend class BadScriptCtrl;
public:
    BadScriptData (char const * title, char const * name, char const * path, char const * notice)
        : _title (title), _name (name), _path (path), _notice (notice)
    {}
    
private:
    char const * _title;
    char const * _name;
    char const * _path;
    char const * _notice;
};

class BadScriptCtrl : public Dialog::ControlHandler
{
public:
    BadScriptCtrl (BadScriptData * data);

    bool OnInitDialog () throw (Win::Exception);
    bool OnApply () throw ();

private:
    Win::StaticText _notice;
    Win::Edit       _name;
    Win::Edit       _path;

    BadScriptData *_dlgData;
};

#endif
