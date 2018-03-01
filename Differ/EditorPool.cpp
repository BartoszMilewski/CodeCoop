//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------
#include "precompiled.h"
#include "EditorPool.h"
#include "Lines.h"
#include "Resource.h"
#include "Registry.h"
#include "EditParams.h"
#include <Win/WinMaker.h>
#include <Win/Message.h>

DocEditor::DocEditor (FileSelection tag, Win::Dow::Handle parentWin, unsigned childId)
: _ctrl (true), _tag (tag)
{
	Win::ChildMaker editWinMaker (IDC_EDIT, parentWin, childId);
	editWinMaker.Style () << Win::Style::AddVScrollBar << Win::Style::AddHScrollBar;
	_win = editWinMaker.Create (&_ctrl);

	// Set editor preferences
	Registry::UserDifferPrefs prefs;
	Font::Maker regFont;
	unsigned tabSize = 4;
	if (prefs.GetFont (tabSize, regFont))
	{
		Win::UserMessage msg (UM_CHANGE_FONT, tabSize, reinterpret_cast<long> (&regFont));
		_win.SendMsg (msg);
	}

	Win::UserMessage msg (UM_TOGGLE_LINE_BREAKING, prefs.IsLineBreakingOn ());
	_win.SendMsg (msg);
}

EditorPool::EditorPool ()
{
	Registry::UserDifferPrefs prefs;
	_lineBreakingOn = prefs.IsLineBreakingOn ();
}

Win::Dow::Handle EditorPool::GetEditableWindow () const
{
	DocEditor * edit = FindDocEditor (FileCurrent);
	return edit? edit->GetWindow (): Win::Dow::Handle ();
}

void EditorPool::Init (Win::Dow::Handle parentWin)
{
	_parentWin = parentWin;
	// Create dummy editor: we need the window for positioning
	std::unique_ptr<DocEditor> editor (new DocEditor (FileMaxCount, _parentWin, ID_EDIT_WINDOW + FileMaxCount));
	_editors.push_back (std::move(editor));

	Win::Dow::Handle win = _editors.back ()->GetWindow ();
	// send message to parent to set editor window
	Win::UserMessage msg (UM_SET_CURRENT_EDIT_WIN);
	msg.SetLParam (win);
	_parentWin.SendMsg (msg);

	_diffEditor.reset (new DocEditor (FileMaxCount, _parentWin, ID_DIFF_WINDOW));
}

void EditorPool::SwitchWindow (FileSelection sel)
{
	DocEditor * edit = FindDocEditor (sel);
	Assert (edit != 0);
	// send message to parent to set editor window
	Win::UserMessage msg (UM_SET_CURRENT_EDIT_WIN);
	msg.SetLParam (edit->GetWindow ());
	_parentWin.SendMsg (msg);
}

void EditorPool::InitDocument (FileSelection sel, std::unique_ptr<EditBuf> editBuf)
{
	DocEditor * editor = FindDocEditor (sel);
	if (editor == 0)
	{
		// First time
		editor = FindDocEditor (FileMaxCount);
		Assert (editor != 0);
		editor->SetTag (sel);
	}
	Win::UserMessage msg (UM_INITDOC);
	msg.SetLParam (&editBuf);
	editor->GetWindow ().SendMsg (msg);
	editor->MakeEditable (sel == FileCurrent);
}

void EditorPool::RemoveDocument (FileSelection sel)
{
	auto_vector<DocEditor>::iterator it = _editors.begin ();
	while (it != _editors.end ())
	{
		if ((*it)->GetTag () == sel)
		{
			_editors.erase (it);
			break;
		}
		++it;
	}
}

void EditorPool::AddDocument (FileSelection sel, std::unique_ptr<EditBuf> editBuf)
{
	DocEditor * editor = FindDocEditor (sel);
	Win::Dow::Handle win;
	if (editor != 0)
	{
		win = editor->GetWindow ();
	}
	else
	{
		std::unique_ptr<DocEditor> newEditor (new DocEditor (sel, _parentWin, ID_EDIT_WINDOW + sel));
		_editors.push_back (std::move(newEditor));
		editor = _editors.back ();
		win = editor->GetWindow ();
		win.Hide ();
	}
	
	Win::UserMessage msg (UM_INITDOC);
	msg.SetLParam (&editBuf);
	win.SendMsg (msg);
	editor->MakeEditable (sel == FileCurrent);
}

void EditorPool::SetEditableReadOnly (bool isReadOnly)
{
	Win::Dow::Handle projWin = GetEditableWindow ();
	if (!projWin.IsNull ())
	{
		Win::UserMessage msg (UM_SET_EDIT_READONLY, isReadOnly ? 1 : 0);
		projWin.SendMsg (msg);
	}
}

void EditorPool::SetLineBreaking (bool on)
{
	_lineBreakingOn = on;

	Win::UserMessage msg (UM_TOGGLE_LINE_BREAKING, _lineBreakingOn);
	SendEditorMsg (msg);
}

void EditorPool::ToggleLineBreaking ()
{
	SetLineBreaking (!IsLineBreakingOn ());
}

void EditorPool::FontChange (unsigned tabSize, Font::Maker const & newFont)
{
	Win::UserMessage msg (UM_CHANGE_FONT, tabSize, reinterpret_cast<long> (&newFont));
	SendEditorMsg (msg);
}

void EditorPool::SetTabSize (unsigned tabSize)
{
	Win::UserMessage msg (UM_CHANGE_FONT, tabSize, 0);
	SendEditorMsg (msg);
}

void EditorPool::SendEditorMsgExcept (Win::Message & msg, Win::Dow::Handle exceptWin)
{
	auto_vector<DocEditor>::const_iterator it = _editors.begin ();
	Win::Dow::Handle win;
	while (it != _editors.end ())
	{
		win = (*it)->GetWindow ();
		if (win != exceptWin)
			win.SendMsg (msg);
		++it;
	}
	win = _diffEditor->GetWindow ();
	if (win != exceptWin)
		win.SendMsg (msg);
}

DocEditor * EditorPool::FindDocEditor (FileSelection sel) const
{
	auto_vector<DocEditor>::const_iterator it = _editors.begin ();
	while (it != _editors.end ())
	{
		if ((*it)->GetTag () == sel)
			return *it;
		++it;
	}
	return 0;
}
