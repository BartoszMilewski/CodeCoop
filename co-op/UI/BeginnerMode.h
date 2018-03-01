#if !defined (BEGINNERMODE_H)
#define BEGINNERMODE_H
//------------------------------------
//  (c) Reliable Software, 1998 - 2006
//------------------------------------

#include <Ctrl/Static.h>
#include <Ctrl/Button.h>
#include <Graph/Icon.h>
#include <Win/Dialog.h>

class BeginnerModeData
{
public:
    BeginnerModeData ()
        : _messageIsOn (false)
    {}

	void SetMessage (char const * msg)
	{
		_message.assign (msg);
		_messageIsOn = true;
	}
    void SetMessageStatus (bool flag)
    {
        _messageIsOn = flag;
    }

	char const * GetMessage () const { return _message.c_str (); }
    bool IsMessageOn () const { return _messageIsOn; }

private:
    std::string	_message;
	bool		_messageIsOn;
};

class BeginnerModeCtrl : public Dialog::ControlHandler
{
public:
    BeginnerModeCtrl (BeginnerModeData * data);

    bool OnInitDialog () throw (Win::Exception);
    bool OnApply () throw ();

private:
    Win::CheckBox		_show;
	Win::StaticText		_message;
	Win::StaticImage	_image;
	Icon::StdMaker		_sysIcon;
    BeginnerModeData *	_dlgData;
};

class BeginnerMode
{
public:
    BeginnerMode ();

    void Display (int msgId, bool isForce = false);
	void SetStatus (bool flag);
	bool IsModeOn () const { return _isModeOn; }
	void On ()  { _isModeOn = true; }
	void Off () { _isModeOn = false; }

private:
    BeginnerModeData	_dlgData;
	int					_disabledCount;
	bool				_isModeOn;
};

#endif
