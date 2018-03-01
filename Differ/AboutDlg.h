#if !defined (ABOUTDLG_H)
#define ABOUTDLG_H
//------------------------------------
//  (c) Reliable Software, 1998 - 2007
//------------------------------------

#include <Ctrl/Static.h>
#include <Win/Dialog.h>

class FileNames;

class HelpAboutData
{
public:
	HelpAboutData (FileNames const & fileNames);
	char const * GetMsg () { return _msg.c_str (); }

private:
	std::string	_msg;
};

class HelpAboutCtrl : public Dialog::ControlHandler
{
public:
    HelpAboutCtrl (HelpAboutData & data);

    bool OnInitDialog () throw (Win::Exception);
    bool OnApply () throw ();

private:
	Win::StaticText	_msg;
    HelpAboutData & _dlgData;
};

#endif
