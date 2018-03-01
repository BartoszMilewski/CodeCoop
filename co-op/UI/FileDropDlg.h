#if !defined (FILEDROPDLG_H)
#define FILEDROPDLG_H
//-----------------------------------
//  (c) Reliable Software 2001 - 2006
//-----------------------------------

#include "resource.h"

#include <Ctrl/Static.h>
#include <Ctrl/Button.h>
#include <Graph/Icon.h>
#include <Win/Dialog.h>

class QuestionData;

class DropQuestionCtrl : public Dialog::ControlHandler
{
public:
    DropQuestionCtrl (QuestionData * data, int dlgId)
		: Dialog::ControlHandler (dlgId),
		  _dlgData (data)
	{}

    bool OnInitDialog () throw (Win::Exception);
    bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

private:
	Win::Button	_ok;
	Win::Button	_yesToAll;
	Win::Button	_noToAll;
	Win::Button	_no;

protected:
	Win::StaticText	_info;
    QuestionData *	_dlgData;
};

class FileDropQuestionCtrl : public DropQuestionCtrl
{
public:
    FileDropQuestionCtrl (QuestionData * data, bool isMultiFileDrop);

    bool OnInitDialog () throw (Win::Exception);

private:
	Win::StaticImage	_targetIcon;
	Win::StaticText		_targetSize;
	Win::StaticText		_targetDate;
	Win::StaticImage	_sourceIcon;
	Win::StaticText		_sourceSize;
	Win::StaticText		_sourceDate;
};

class FolderDropQuestionCtrl : public DropQuestionCtrl
{
public:
    FolderDropQuestionCtrl (QuestionData * data, bool isMultiFileDrop);

    bool OnInitDialog () throw (Win::Exception);
};

class AddDropQuestionCtrl : public DropQuestionCtrl
{
public:
    AddDropQuestionCtrl (QuestionData * data, bool isMultiFileDrop)
		: DropQuestionCtrl (data, isMultiFileDrop ? IDD_ADD_DROP : IDD_ADD_ONE_DROP)
	{}

    bool OnInitDialog () throw (Win::Exception);
};

#endif
