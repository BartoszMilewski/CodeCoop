// ---------------------------
// (c) Reliable Software, 2006
// ---------------------------
#include "precompiled.h"
#include "Prompter.h"
#include "AlertMan.h"
#include "OutputSink.h"

#include <Win/Dialog.h>
#include <Ctrl/PropertySheet.h>

Prompter ThePrompter;

bool Prompter::Prompt (std::string const & msg, char const * alert)
{
	Assert (_isOnUserCmd != 0);
	Assert (alert != 0);
	if (!IsOnUserCmd ())
	{
		TheAlertMan.PostQuarantineAlert (alert);
		return false;
	}

	Out::Answer answer = TheOutput.Prompt (msg.c_str (), Out::PromptStyle (Out::YesNo));
	return answer == Out::Yes;
}

// Returns true when OK button clicked
bool Prompter::GetData (Dialog::ControlHandler & ctrl, char const * alert)
{
	Assert (_isOnUserCmd != 0);
	if (!IsOnUserCmd ())
	{
		if (alert != 0)
		{
			TheAlertMan.PostQuarantineAlert (alert);
			return false;
		}
	}
	Dialog::Modal dialog (_appWin, ctrl);
	return dialog.IsOK ();
}

// Returns true when OK button clicked
bool Prompter::GetSheetData (PropPage::HandlerSet & ctrlSet)
{
	Assert (_isOnUserCmd != 0);
	PropPage::Sheet propertySheet (_appWin, ctrlSet.GetCaption ());
	propertySheet.SetNoApplyButton ();
	propertySheet.SetNoContextHelp ();
	for (PropPage::HandlerSet::Sequencer seq (ctrlSet); !seq.AtEnd (); seq.Advance ())
	{
		propertySheet.AddPage (seq.GetPageHandler (), seq.GetPageCaption ());
	}
	propertySheet.SetStartPage (ctrlSet.GetStartPage ());
	return propertySheet.Display ();
}

// Returns true when OK button clicked
bool Prompter::GetWizardData (PropPage::HandlerSet & ctrlSet, char const * alert)
{
	Assert (_isOnUserCmd != 0);
	if (!IsOnUserCmd ())
	{
		if (alert != 0)
		{
			TheAlertMan.PostQuarantineAlert (alert);
			return false;
		}
	}

	PropPage::Wizard wizard (_appWin, ctrlSet.GetCaption ());
	for (PropPage::HandlerSet::Sequencer seq (ctrlSet); !seq.AtEnd (); seq.Advance ())
	{
		wizard.AddPage (seq.GetPageHandler ());
	}
	return wizard.Run ();
}
