//----------------------------------
// (c) Reliable Software 1998 - 2008
//----------------------------------

#include "precompiled.h"
#include "Prompter.h"
#include "InputSource.h"
#include "JoinProjectData.h"
#include "OpenSaveFileData.h"

#include <Win/Dialog.h>
#include <Graph/Cursor.h>
#include <Sys/Synchro.h>
#include <Ctrl/PropertySheet.h>

Prompter ThePrompter;

bool JoinDataExtractor::GetDataFrom (NamedValues const & namedValues)
{
	_data.ReadNamedValues (namedValues);
	return _data.IsValid ();
}

bool BackupRequestExtractor::GetDataFrom (NamedValues const & namedValues)
{
	_data.ReadNamedValues (namedValues);
	return _data.IsValid();
}

// Returns true when OK button clicked
bool Prompter::GetData (Dialog::ControlHandler & ctrl, InputSource * alternativeInput)
{
	if (alternativeInput == 0)
	{
		Cursor::Arrow arrow;
		Cursor::Holder cursorSwitch (arrow);
		ctrl.AttachHelp (&_helpEngine);
		Assert (Dbg::IsMainThread ());
		Win::UnlockPtr unlock (_critSect);
		Dialog::Modal dialog (_win, ctrl);
		return dialog.IsOK ();
	}
	else
	{
		// let ctrl retrieve data from command line input
		return ctrl.GetDataFrom (alternativeInput->GetNamedArguments ());
	}
}

// Returns true when OK button clicked
bool Prompter::GetData (PropPage::HandlerSet & ctrlSet, InputSource * alternativeInput)
{
	if (alternativeInput == 0)
	{
		Cursor::Arrow arrow;
		Cursor::Holder cursorSwitch (arrow);
		Assert (Dbg::IsMainThread ());
		PropPage::Sheet propertySheet (_win, ctrlSet.GetCaption ());
		propertySheet.SetNoApplyButton ();
		propertySheet.SetNoContextHelp ();
		for (PropPage::HandlerSet::Sequencer seq (ctrlSet); !seq.AtEnd (); seq.Advance ())
		{
			propertySheet.AddPage (seq.GetPageHandler (), seq.GetPageCaption ());
		}
		propertySheet.SetStartPage (ctrlSet.GetStartPage ());
		return propertySheet.Display (_critSect);
	}
	else
	{
		return ctrlSet.GetDataFrom (alternativeInput->GetNamedArguments ());
	}
}

bool Prompter::GetData (Extractor & extr, InputSource * alternativeInput)
{
	bool result = false;
	NamedValues const & src = alternativeInput? 
		alternativeInput->GetNamedArguments ()
		: _argCache;
    return extr.GetDataFrom (src);
}

