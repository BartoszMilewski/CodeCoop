#if !defined (GOTOLINEDLG_H)
#define GOTOLINEDLG_H
//------------------------------------
//  (c) Reliable Software, 2001 - 2005
//------------------------------------

#include <Win/Dialog.h>
#include <Ctrl/Button.h>
#include <Ctrl/Edit.h>
#include <Win/Win.h>

class GoToLineDlgData
{
public :
	GoToLineDlgData (int paraNo, int lastParaNo)
		: _paraNo (paraNo), _lastParaNo (lastParaNo)
	{}
	int GetParaNo () const { return _paraNo; }
	int GetLastParaNo () const { return _lastParaNo; }
	void SetParaNo (int paraNo) { _paraNo = paraNo; }

private:
	int _paraNo;
	int _lastParaNo;
};

class GoToLineDlgController : public Dialog::ControlHandler
{
public:
	GoToLineDlgController (GoToLineDlgData & dlgData);

	bool OnInitDialog () throw (Win::Exception);
	bool OnApply () throw ();

private:
	Win::Edit          _paraNo;	
	Win::EditReadOnly  _lastParaNo;
	GoToLineDlgData &  _dlgData;	
};

#endif
