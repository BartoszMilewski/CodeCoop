#if !defined (PROMPTMODE_H)
#define PROMPTMODE_H
//------------------------------------
//  (c) Reliable Software, 2004 - 2005
//------------------------------------

#include <Ctrl/Static.h>
#include <Graph/Icon.h>
#include <Ctrl/Button.h>
#include <Win/Dialog.h>

class PromptModeData
{
public:
    PromptModeData (unsigned int msgId);

	void TurnOffPrompt ();

	std::string const & GetPrompt () const { return _prompt; }
    bool IsOn () const { return _isOn; }

private:
	unsigned int	_id;
    std::string		_prompt;
	bool			_isOn;
};

class PromptModeCtrl : public Dialog::ControlHandler
{
public:
    PromptModeCtrl (PromptModeData * data);

    bool OnInitDialog () throw (Win::Exception);
    bool OnApply () throw ();
	bool OnCancel () throw ();

private:
    Win::CheckBox		_dontShowAgain;
	Win::StaticText		_prompt;
	Win::StaticImage	_promptImage;
	Icon::StdMaker		_sysIcon;
    PromptModeData *	_dlgData;
};

class PromptMode
{
public:
    PromptMode (unsigned int promptId)
		: _dlgData (promptId)
	{}

	bool CanPrompt () const { return _dlgData.IsOn (); }
    bool Prompt ();

private:
    PromptModeData	_dlgData;
};


#endif
