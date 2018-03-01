//------------------------------------
//  (c) Reliable Software, 2000 - 2005
//------------------------------------
#include "precompiled.h"
#include "FindDlg.h"
#include "EditParams.h"
#include "resource.h"
#include "Registry.h"

#include <Ctrl/Output.h>
#include <Ctrl/Accelerator.h>
#include <Win/Notification.h>
#include <Ex/WinEx.h>
#include <Ctrl/Button.h>
#include <Dbg/Assert.h>

// Find dialog data

bool FindDialogData::SetCanEdit (bool canEdit)
{
	if (canEdit == _canEdit)
		return false;

	_canEdit = canEdit;
	return true;
}

void FindDialogData::RememberDlgPosition (int xLeft, int yTop)
{
	if (_isOnePane)
	{
		_onePanePosition.RememberXLeft (xLeft);
		_onePanePosition.RememberYTop (yTop);
	}
	else
	{
		_twoPanePosition.RememberXLeft (xLeft);
		_twoPanePosition.RememberYTop (yTop);
	}
}

bool FindDialogData::GetDlgPosition (int & xLeft, int & yTop)
{
	if (_isOnePane)
	{
		xLeft = _onePanePosition.GetXLeft ();
		yTop = _onePanePosition.GetYTop ();
	}
	else
	{
		xLeft = _twoPanePosition.GetXLeft ();
		yTop = _twoPanePosition.GetYTop ();
	}
	return _dialogWasOpen;
}

void FindDialogData::RememberDlgPositions (int xLeft1, int yTop1, int xLeft2, int yTop2)
{
	_onePanePosition.RememberXLeft (xLeft1);
	_onePanePosition.RememberYTop (yTop1);
	_twoPanePosition.RememberXLeft (xLeft2);
	_twoPanePosition.RememberYTop (yTop2);
}

// Associations

bool ReplaceDialogData::Associations::GetAssociation (const std::string & key, std::string & value)
{
	std::list <std::string>::iterator itKey = _keyList.begin ();
	std::list <std::string>::iterator itValue = _valueList.begin ();
	for (; itKey != _keyList.end () && itValue != _valueList.end (); ++itKey, ++itValue)
	{
		if (*itKey == key)
		{
			value = *itValue;
			return true;					
		}
	}
	return false;
}

void ReplaceDialogData::Associations::SetAssociation (const std::string & key, const std::string & value)
{
	Assert (!key.empty ());
	std::list <std::string>::iterator itKey = _keyList.begin ();
	std::list <std::string>::iterator itValue = _valueList.begin ();
	for (; itKey != _keyList.end () && itValue != _valueList.end (); ++itKey, ++itValue)
	{
		if (*itKey == key)
		{
			*itValue = value;
			return;					
		}
	}
	_keyList.push_front (key);
	_valueList.push_front (value);
}

// Replace dialog data

bool ReplaceDialogData::GetAssociation (const std::string & key, std::string & value)
{
	return _assotiations.GetAssociation (key, value);
}

void ReplaceDialogData::SetAssociation (const std::string & key, const std::string & value)
{
	_assotiations.SetAssociation (key, value);
}

// Find dialog controller

FindDialogCtrl::FindDialogCtrl (FindDialogData & dlgData, int dlgId)
	: Dialog::ControlHandler (dlgId),
	  _dlgData (dlgData)
{}

bool FindDialogCtrl::OnInitDialog () throw (Win::Exception)
{
	_findCombo.Init (GetWindow (), IDC_FIND_WORD);
	_findNext.Init (GetWindow (), IDC_FIND_NEXT);
	_findReplace.Init (GetWindow (), IDC_FIND_REPLACE);
	_close.Init (GetWindow (),IDC_FIND_CLOSE);
	_down.Init (GetWindow (), IDC_FIND_DOWN);
	_up.Init (GetWindow (), IDC_FIND_UP);
	_matchWholeWord.Init (GetWindow (), IDC_FIND_WHOLE_WORD);
	_matchCase.Init (GetWindow (), IDC_FIND_MATCH_CASE);

	SetDialogState ();
	std::unique_ptr<Win::ComboBox::NotifyHandler>  handler (new FindHandler(this, IDC_FIND_WORD));
	_findCombo.SetNotifyHandler (std::move(handler));
	return true;
}

bool FindDialogCtrl::OnApply () throw ()
{
	EndOk ();
	return true;
}

void FindDialogCtrl::SetDialogState ()
{
	std::list<std::string> stringList;
	Registry::UserDifferPrefs regPrefs;
	regPrefs.GetFindList (stringList);
	FillListBox (stringList, _findCombo);
	_findCombo.SetEditText (_dlgData.GetFindWord ().c_str ());
	
	if (_dlgData.IsDirectionForward ())
	{
		_up.UnCheck ();
		_down.Check ();
	}
	else
	{
		_up.Check ();
		_down.UnCheck ();
	}

	if (_dlgData.IsMatchCase ())
		_matchCase.Check ();
	if (_dlgData.IsWholeWord ())
		_matchWholeWord.Check ();
	SetEditStatus ();
}

void FindDialogCtrl::SetEditStatus ()
{
	if (!GetWindow ().IsNull ())
	{
		if (_dlgData.CanEdit ())
		{
			_findReplace.Enable ();
		}
		else
		{			
			_findReplace.Disable ();
		}
	}
}

void FindDialogCtrl::OnActivate () throw (Win::Exception) 
{	
	SetEditStatus ();
}

bool FindDialogCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_FIND_DOWN:
	case IDC_REPLACE_DOWN:
		_dlgData.SetDirectionForward (true);
		SetDialogState ();	//	because we might be called programmatically
		return true;
	case IDC_FIND_UP:
	case IDC_REPLACE_UP:
		_dlgData.SetDirectionForward (false);
		SetDialogState ();	//	because we might be called programmatically
		return true;
	case IDC_FIND_WHOLE_WORD:
    case IDC_REPLACE_WHOLE_WORD:
		if (_dlgData.IsWholeWord ())
			_dlgData.SetWholeWord (false);
		else 			
			_dlgData.SetWholeWord (true);
		return true;
	case IDC_FIND_MATCH_CASE :
	case IDC_REPLACE_MATCH_CASE :
		if (_dlgData.IsMatchCase())
			_dlgData.SetMatchCase (false);
		else 			
			_dlgData.SetMatchCase (true);
		
		return true;
	case IDC_FIND_NEXT:
    case IDC_REPLACE_FIND_NEXT:
		{
			_dlgData.SetFindWord (_findCombo.RetrieveEditText ());
			if (!_dlgData.GetFindWord ().empty ())
			{
				RefreshList (_findCombo, _dlgData.GetFindWord ());
				Win::Dow::Handle mainWin = GetWindow ().GetOwner ();
				Win::UserMessage msg (UM_FIND_NEXT);
				msg.SetLParam (static_cast<SearchRequest *> (&_dlgData));
				mainWin.SendMsg (msg);
			}
			return true;
		}
	case IDC_FIND_REPLACE:
		{
			_dlgData.SetFindWord (_findCombo.RetrieveEditText ());
			Win::Dow::Handle mainWin = GetWindow ().GetOwner ();
			Win::UserMessage msg (UM_INIT_DLG_REPLACE);
			mainWin.SendMsg (msg);
			return true;
		}
		
	case IDC_FIND_CLOSE:
		EndCancel ();
		return true;
	}
    return false;
}

bool FindDialogCtrl::OnCommand (int cmdId, bool isAccel) throw ()
{
	if (cmdId == Out::Cancel)
	{
		EndCancel ();
		return true;
	}
	return false;
}

void FindDialogCtrl::OnDestroy () throw  () 
{ 
	CloseDialog ();
}

void FindDialogCtrl::CloseDialog () throw ()
{
	if (_dlgData.GetDialog ().IsNull ())
		return;
	try
	{
		Win::Placement dlgPlacement (_dlgData.GetDialog ());
		Win::Rect rect;
		dlgPlacement.GetRect (rect);
		_dlgData.RememberDlgPosition (rect.left, rect.top);
		_dlgData.ResetDialog ();
		std::list<std::string> stringList;	
		RetriveStringList (stringList, _findCombo);
		Registry::UserDifferPrefs regPrefs;
		regPrefs.SaveFindList (stringList);
		regPrefs.SaveFindPref (_dlgData.IsWholeWord (), _dlgData.IsMatchCase ());
	}
	catch (...)
	{
		Win::ClearError ();
	}
}

void FindDialogCtrl::RetriveStringList (std::list <std::string> & list, Win::ComboBox & box)
{			
	int count = box.Count ();
	for (int k = 0; k < count; k++)
		list.push_back (box.RetrieveText (k));
}

void FindDialogCtrl::FillListBox (std::list <std::string> & list, Win::ComboBox & box)
{
	std::list<std::string>::iterator it;
	for (it = list.begin (); it != list.end (); it++)
		box.AddToList (it->c_str ());
}

// bellow function set newItem in edit box ,set newItem on the top of list
//and remowe  the duplicat (case sensitive), if exist 
void FindDialogCtrl::RefreshList (Win::ComboBox & box, const std::string & newItem)
{
	int findIdx = box.FindStringExact (newItem.c_str ());//This function is not case sensitive	
	while (findIdx != -1)
	{
		std::string word (box.RetrieveText (findIdx));
		if (word == newItem)
		{
			if (findIdx == 0)
				return;//finding string ist first on the list - nothing to do
			else
			{
				box.Remove (findIdx);//remove duplicat
				box.SetEditText (newItem.c_str ());
				break;
			}
		}
		else
		{
			int idxOld  = findIdx;
			findIdx = box.FindStringExact (newItem.c_str (), idxOld);
			if (idxOld <= findIdx)// pass the end of list and continue search at begin
				break;
		}
	}
	box.Insert (0, newItem.c_str () );//insert  last finding string on top
}

// Replace dialog controller

ReplaceDialogCtrl::ReplaceDialogCtrl (ReplaceDialogData & dlgData, int dlgId)
	: FindDialogCtrl (dlgData, dlgId),
	  _replaceDlgData (dlgData)
{}

bool ReplaceDialogCtrl::OnInitDialog () throw (Win::Exception)
{	
	_findCombo.Init (GetWindow (), IDC_REPLACE_WHAT);
	_findNext.Init (GetWindow (), IDC_REPLACE_FIND_NEXT);
	_close.Init (GetWindow (),IDC_REPLACE_CLOSE);
	_down.Init (GetWindow (), IDC_REPLACE_DOWN);
	_up.Init (GetWindow (), IDC_REPLACE_UP);
	_matchWholeWord.Init (GetWindow (), IDC_REPLACE_WHOLE_WORD);
	_matchCase.Init (GetWindow (), IDC_REPLACE_MATCH_CASE);
	_hint.Init (GetWindow (), IDC_REPLACE_HINT);
	_replace.Init (GetWindow (), IDC_REPLACE);
	_replaceAll.Init (GetWindow (), IDC_REPLACE_ALL);
	_replaceCombo.Init (GetWindow (), IDC_REPLACE_WITH);	

    std::unique_ptr<Win::ComboBox::NotifyHandler> handlerF (new FindHandler (this, IDC_REPLACE_WHAT));
	_findCombo.SetNotifyHandler (std::move(handlerF));
	std::unique_ptr<Win::ComboBox::NotifyHandler> handlerS (new FindHandler (this, IDC_REPLACE_WITH));
	_replaceCombo.SetNotifyHandler (std::move(handlerS));
	SetDialogState ();
	return true;
}

void ReplaceDialogCtrl::SetDialogState ()
{
	FindDialogCtrl::SetDialogState ();

	Registry::UserDifferPrefs regPrefs;
	std::list<std::string> stringList;
	regPrefs.GetReplaceList (stringList);
	regPrefs.GetAssociationList (_replaceDlgData.AssocKeyList (), _replaceDlgData.AssocValueList ());
	FillListBox  (stringList, _replaceCombo);
	if (_replaceDlgData.IsHintOn ())
		_hint.Check ();
	if ( (!_replaceDlgData.IsHintOn () || !ShowHint ())
		&& _replaceDlgData.GetSubstitution ().empty () && !stringList.empty ())
	{
		_replaceDlgData.SetSubstitution (stringList.front ());
	}	
	_replaceCombo.SetEditText (_replaceDlgData.GetSubstitution ().c_str ());
	SetEditStatus ();
}

bool ReplaceDialogCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_REPLACE_DOWN:
	case IDC_REPLACE_UP:	
	case IDC_REPLACE_WHOLE_WORD:
	case IDC_REPLACE_MATCH_CASE :
	case IDC_REPLACE_FIND_NEXT:
	case IDC_FIND_NEXT:
		return FindDialogCtrl::OnDlgControl (ctrlId, notifyCode);	
	case IDC_REPLACE_WHAT:
		_findCombo.OnNotify (notifyCode);		
		return true;
	case IDC_REPLACE_HINT:
		if (_replaceDlgData.IsHintOn ())
			_replaceDlgData.ToggleHint (false);
		else 			
			_replaceDlgData.ToggleHint (true);

		_replaceDlgData.SetFindWord (_findCombo.RetrieveEditText ());
		if (_replaceDlgData.IsHintOn ())
			ShowHint ();		
		return true;
	case IDC_REPLACE:
	case IDC_REPLACE_ALL:
		{
			_replaceDlgData.SetFindWord (_findCombo.RetrieveEditText ());
			if (!_replaceDlgData.GetFindWord ().empty ())
			{
				_replaceDlgData.SetSubstitution (_replaceCombo.RetrieveEditText ());
				Win::Dow::Handle mainWin = GetWindow ().GetOwner ();
				Win::UserMessage msg (UM_REPLACE);
				if (ctrlId == IDC_REPLACE_ALL)
					msg = Win::UserMessage (UM_REPLACE_ALL);
				msg.SetLParam (static_cast<ReplaceRequest *> (& _replaceDlgData));
				mainWin.SendMsg (msg);
				RefreshList (_findCombo, _replaceDlgData.GetFindWord ());
				RefreshList (_replaceCombo, _replaceDlgData.GetSubstitution ());
				_replaceDlgData.SetAssociation (_replaceDlgData.GetFindWord (),_replaceDlgData.GetSubstitution ());
			}
			return true;
		}
	case IDC_REPLACE_CLOSE:
		EndCancel ();
		return true;
	}
    return false;
}

void  ReplaceDialogCtrl::OnEditChange (int idBox)
{
	if (idBox == IDC_REPLACE_WHAT)
	{
		_replaceDlgData.SetFindWord (_findCombo.RetrieveEditText ());
		if (_replaceDlgData.IsHintOn ())
			ShowHint ();
	}
}

void ReplaceDialogCtrl::OnSelChange (int idBox)
{
	if (idBox == IDC_REPLACE_WHAT)
	{
		int idxSel = _findCombo.GetSelectionIdx ();		 
		if (idxSel != -1)
		{
			_replaceDlgData.SetFindWord (_findCombo.RetrieveText (idxSel));
			if (_replaceDlgData.IsHintOn ())
				ShowHint ();
		}
	}
}

void ReplaceDialogCtrl::OnActivate () throw (Win::Exception) 
{	
	SetEditStatus ();
}

void ReplaceDialogCtrl::SetEditStatus ()
{
	if (!GetWindow ().IsNull ())
	{
		if (_replaceDlgData.CanEdit ())
		{
			_replace.Enable ();
			_replaceAll.Enable ();			
		}
		else
		{			
			_replace.Disable ();
			_replaceAll.Disable ();
		}
	}
}

bool ReplaceDialogCtrl::ShowHint ()
{
	// first remmember old text
	_replaceDlgData.SetSubstitution (_replaceCombo.RetrieveEditText ());
	if (!_replaceDlgData.GetSubstitution ().empty ())
		RefreshList (_replaceCombo, _replaceDlgData.GetSubstitution ());
    // now display hint
	if (_replaceDlgData.GetAssociation (_replaceDlgData.GetFindWord (), _replaceDlgData.GetSubstitution ()))
	{
		_replaceCombo.SetEditText (_replaceDlgData.GetSubstitution ().c_str ());
		RefreshList (_replaceCombo, _replaceDlgData.GetSubstitution ());
		return true;
	}
	else
	{
		_replaceCombo.SetEditText ("");
		return false;
	}	
}

void ReplaceDialogCtrl::OnDestroy () throw  () 
{ 
	CloseDialog ();
}

void ReplaceDialogCtrl::CloseDialog () throw ()
{
	if (_dlgData.GetDialog ().IsNull ())
		return;
	try
	{
		std::list <std::string> transferlist;
		RetriveStringList(transferlist, _replaceCombo);
		Registry::UserDifferPrefs regPrefs;
		regPrefs.SaveReplaceList (transferlist);
		regPrefs.SaveAssociationList (_replaceDlgData.AssocKeyList (), _replaceDlgData.AssocValueList ());
		_replaceDlgData.ClearAssociations ();
		FindDialogCtrl::CloseDialog ();
	}
	catch (...)
	{
		Win::ClearError ();
	}
}

//----------FindPrompter----------

FindPrompter::FindPrompter (Win::MessagePrepro & msgPrepro, Win::Dow::Handle parentWin, Accel::Handle accel)
	: _msgPrepro (msgPrepro),
	  _findCtrl (_commonData, IDD_FIND_IN_FILE),
	  _replaceCtrl (_commonData, IDD_REPLACE_IN_FILE),
	  _parentWin (parentWin),
	  _accel (accel)
{}

void FindPrompter::InitDialogFind (Win::Dow::Handle & focusWin)
{
	InitDialog (focusWin, IDD_FIND_IN_FILE);
}

void FindPrompter::InitDialogReplace (Win::Dow::Handle & focusWin)
{
	InitDialog (focusWin, IDD_REPLACE_IN_FILE);
}

void FindPrompter::InitDialog (Win::Dow::Handle & focusWin, int idDialog)
{
	_commonData.Activate ();
	// close existing dialog (if it exists)	
	if (_commonData.IsOpenDialog ())
		_commonData.GetDialog ().Destroy ();

    // check selection in text
	Win::UserMessage msg (UM_GET_SELECTION);
	msg.SetLParam (static_cast<SearchRequest *> (& _commonData));
	focusWin.SendMsg (msg);

	// create new dialog (find or replace)
	Dialog::Handle  dlg;
	if (idDialog == IDD_FIND_IN_FILE)
	{
		Dialog::ModelessMaker maker (_findCtrl, _msgPrepro, _parentWin, _accel);
		dlg = maker.Create (_parentWin);
	}
	else
	{
		Assert (idDialog == IDD_REPLACE_IN_FILE);
		Dialog::ModelessMaker maker (_replaceCtrl, _msgPrepro, _parentWin, _accel);
		dlg = maker.Create (_parentWin);
	}
	_commonData.SetDialog (dlg);

    // set dialog position
	int xLeft,yTop;
	_commonData.GetDlgPosition (xLeft, yTop);
	Win::Placement dlgPlacement (dlg);
	Win::Rect rect;
	dlgPlacement.GetRect (rect);
	int width = rect.Width ();
	int height = rect.Height ();
	rect = Win::Rect (xLeft, yTop, xLeft + width, yTop + height);
	dlgPlacement.SetRect (rect);
	dlg.SetPlacement (dlgPlacement);
	dlg.Show ();
}

void FindPrompter::RememberDlgPositions (int xLeft1, int yTop1, int xLeft2, int yTop2) 
{
	_commonData.RememberDlgPositions (xLeft1, yTop1, xLeft2, yTop2);
}

bool FindPrompter::GetDlgPosition (int & xLeft, int & yTop)
{
	return _commonData.GetDlgPosition (xLeft, yTop);
}

void FindPrompter::SetPaneMode (bool isOnePane)
{
	if (_commonData.IsOpenDialog ())
	{
		Win::Placement dlgPlacement (_commonData.GetDialog ());
		Win::Rect rect;
		dlgPlacement.GetRect (rect);
		_commonData.RememberDlgPosition (rect.left, rect.top);
		_commonData.SetPaneMode (isOnePane);
		int xLeft,yTop;
		_commonData.GetDlgPosition (xLeft,yTop);	
		_commonData.GetDialog ().Move (xLeft, yTop, rect.Width (), rect.Height ());
	}
	else
		_commonData.SetPaneMode (isOnePane);
}

// Used when FindNext is selected from the menu (or a kbd accelerator)
void FindPrompter::FindNext (bool directionBackward)
{
	if (_commonData.IsOpenDialog ())
	{	// execute through controls :
		// first set direction
		Win::ControlMessage directionMsg (directionBackward? IDC_FIND_UP: IDC_FIND_DOWN,
										  Win::Button::Clicked, _commonData.GetDialog ());
		_commonData.GetDialog ().SendMsg (directionMsg);
		// "push" FIND NEXT buton
		Win::ControlMessage msgFindNext (IDC_FIND_NEXT, Win::Button::Clicked, _commonData.GetDialog ());
		_commonData.GetDialog ().SendMsg (msgFindNext);
	}
	else
	{	// dialog is closed, execute directly
		_commonData.SetDirectionForward (!directionBackward);
		Win::UserMessage msg (UM_FIND_NEXT);
		msg.SetLParam (static_cast<SearchRequest *> (&_commonData));
		_parentWin.SendMsg (msg);
	}
}

void FindPrompter::SetCanEdit (bool canEdit)
{
	if (_commonData.SetCanEdit (canEdit))
	{
		_findCtrl.SetEditStatus ();
		_replaceCtrl.SetEditStatus ();
	}
}
