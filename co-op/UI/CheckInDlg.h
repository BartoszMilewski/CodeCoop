#if !defined CHECKINDLG_H
#define CHECKINDLG_H
//------------------------------------
//  (c) Reliable Software, 1997 - 2008
//------------------------------------

#include <Ctrl/Edit.h>
#include <Ctrl/Button.h>
#include <Win/Dialog.h>

class InputSource;

class CheckInData
{
public:
	CheckInData ()
		: _keepCheckedOut (false)
	{}

    void SetComment (std::string const & comment)
    {
        _comment.assign (comment);
		Assert (!_comment.empty ());
    }
	void SetLastRejectedComment (std::string const & comment)
	{
		_lastRejectedComment = comment;
	}
	void SetKeepCheckedOut (bool flag) { _keepCheckedOut = flag; }
	void ClearComment () { _comment.clear (); }
	void UseLastRejectedComment () { _comment = _lastRejectedComment; }
	std::string const & GetLastRejectedComment () const { return _lastRejectedComment; }

	std::string const & GetComment () const { return _comment; }
	bool IsKeepCheckedOut () const { return _keepCheckedOut; }

private:
    std::string _comment;
	std::string _lastRejectedComment;
	bool		_keepCheckedOut;
};

class CheckInCommentCtrl : public Dialog::ControlHandler
{
public:
    CheckInCommentCtrl (CheckInData * data);

    bool OnInitDialog () throw (Win::Exception);
    bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

	bool GetDataFrom (NamedValues const & source);

private:
    Win::Edit			_comment;
	Win::Button			_okButton;
	Win::CheckBox		_lastComment;
	Win::CheckBox		_keepCheckedOut;
	Win::RadioButton	_noTest;
	Win::RadioButton	_someTest;
	Win::RadioButton	_thoroughTest;
	Win::RadioButton	_notApplicableTest;
    CheckInData *		_dlgData;
};

class CheckInNoChangesCtrl : public Dialog::ControlHandler
{
public:
    CheckInNoChangesCtrl (CheckInData * data);
    bool OnInitDialog () throw (Win::Exception);
    bool OnApply () throw ();
	bool GetDataFrom (NamedValues const & source)
	{
		// don't leave it checked out
		return true;
	}
private:
	Win::CheckBox	_keepCheckedOut;
    CheckInData *	_dlgData;
};

class CheckInUI
{
public:
    CheckInUI (Win::Dow::Handle hwnd, InputSource * source)
		: _hwnd (hwnd),
		  _source (source)
    {}
    bool Query (std::string const & lastRejectedComment, bool keepCheckedOut);
	void NoChangesDetected (bool keepCheckedOut);

	std::string const & GetComment () const { return _dlgData.GetComment (); }
	bool IsKeepCheckedOut () const { return _dlgData.IsKeepCheckedOut (); }

private:
    CheckInData		_dlgData;
    Win::Dow::Handle _hwnd;
	InputSource *	_source;
};

#endif
