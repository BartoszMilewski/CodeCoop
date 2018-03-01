//------------------------------------
//  (c) Reliable Software, 2005 - 2008
//------------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "Controllers.h"
#include "resource.h"
#include "UiManager.h"
#include "UiStrings.h"
#include "Prompter.h"
#include "Layout.h"
#include "InstrumentBar.h"
#include "MultiLine.h"
#include "FormattedText.h"
#include "ListObserver.h"

#include <Ctrl/Focus.h>
#include <Ctrl/ComboBox.h>
#include <File/MemFile.h>

//---------------------
// Branch Controller
//---------------------

BranchController::BranchController (int id,
									Win::Dow::Handle win, 
									Focus::Ring & focusRing)
	: Notify::BranchHandler (id),
	  _id (id),
	  _focusRing (focusRing),
	  _kbdHandler (focusRing),
	  _view (win, id),
	  _browser (0)
{}

// Revisit: move to library!

bool BranchController::BranchKbdHandler::OnTab () throw ()
{
	if (IsShift ())
		_focusRing.SwitchToPrevious ();
	else
		_focusRing.SwitchToNext ();
	return true;
}
//---------------------
// Hierarchy Controller
//---------------------

HierarchyController::HierarchyController (int id,
										  Win::Dow::Handle win, 
										  Cmd::Executor & executor, 
										  Focus::Ring & focusRing)
	: Notify::TreeViewHandler (id),
	  _id (id),
	  _executor (executor),
	  _focusRing (focusRing),
	  _kbdHandler (focusRing),
	  _view (win, id),
	  _browser (0),
	  _listObserver (0),
	  _previousDragOverItem (0)
{
	RegisterAsDropTarget(_view);	// Register controller's window as file drop target window
}

bool HierarchyController::IsActive () const
{
	return _browser != 0 && _browser->IsActive ();
}

bool HierarchyController::HierarchyKbdHandler::OnTab () throw ()
{
	if (IsShift ())
		_focusRing.SwitchToPrevious ();
	else
		_focusRing.SwitchToNext ();
	return true;
}

bool HierarchyController::IsInsideView (Win::Point const & screenPt) const
{
	Win::ClientRect clientRect (_view);
	Win::Point clientPt (screenPt);
	_view.ScreenToClient (clientPt);
	return clientRect.IsInside (clientPt);
}

bool HierarchyController::OnClick (Win::Point pt) throw ()
{
	return false;
}

bool HierarchyController::OnRClick (Win::Point pt) throw ()
{
	_view.ScreenToClient (pt);
	Tree::NodeHandle node = _view.HitTest (pt);
	if (!node.IsNull ())
		_view.Select (node);
	return true;
}

bool HierarchyController::OnDblClick (Win::Point pt) throw ()
{
	return false;
}

bool HierarchyController::OnSetFocus (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
{
	_focusRing.SwitchToThis (idFrom);
	return true;
}

bool HierarchyController::OnItemExpanding (Tree::View::Item & item,
										   Tree::Notify::Action action,
										   bool & allow) throw ()
{
	Assert (_browser != 0 && _listObserver != 0);
	if (action == Tree::Notify::Unknown)
		return true;

	Tree::NodeHandle parentNode = item.GetNode ();
	allow = _browser->GetHierarchy ().Expand (parentNode);
	return true;
}

bool HierarchyController::OnItemCollapsed (Tree::View::Item & item,
										   Tree::Notify::Action action) throw ()
{
	Assert (_browser != 0 && _listObserver != 0);
	if (action == Tree::Notify::Unknown)
		return true;

	Tree::NodeHandle parentNode = item.GetNode ();
	int levelsUp = _browser->GetHierarchy ().Collapse (parentNode);
	Assert (levelsUp == 0);
	return true;
}

bool HierarchyController::OnSelChanged (Tree::View::Item & itemOld, 
										Tree::View::Item & itemNew, 
										Tree::Notify::Action action) throw ()
{
	//if (action == Tree::Notify::Unknown)
	//	return true;

	Tree::NodeHandle node = itemNew.GetNode ();
	ChangeCurFolder (node);
	return true;
}

void HierarchyController::OnDragEnter (Win::Point screenDropPoint)
{
	SetDropTargetItem (screenDropPoint);
}

void HierarchyController::OnDragLeave ()
{
	ClearDropTargetItem ();
}

void HierarchyController::OnDragOver (Win::Point screenDropPoint)
{
	SetDropTargetItem (screenDropPoint);
}

void HierarchyController::OnFileDrop (Win::FileDropHandle droppedFiles, Win::Point screenDropPoint)
{
	ClearDropTargetItem ();
	NamedValues args;
	Tree::NodeHandle dropItem = GetHitItem (screenDropPoint);
	if (dropItem.IsNull ())
		return;

	if (_browser->IsCurrentDir (dropItem))
	{
		args.Add ("DropTargetString", "");
	}
	else
	{
		Hierarchy & hierarchy = _browser->GetHierarchy ();
		File::Vpath relPath;
		hierarchy.RetrieveNodePath (dropItem, relPath);
		args.Add ("DropTargetString", relPath.ToString ());
		args.Add ("TargetType", "RootRelative");
	}

	args.Add ("FileDropHandle", ::ToHexString (reinterpret_cast<unsigned int>(droppedFiles.ToNative ())));
	_executor.PostCommand ("DoDrop", args);
}

Tree::NodeHandle HierarchyController::GetHitItem (Win::Point screenDropPoint)
{
	Win::Point viewDropPoint (screenDropPoint);
	_view.ScreenToClient (viewDropPoint);
	return _view.HitTest (viewDropPoint);
}

void HierarchyController::SetDropTargetItem (Win::Point screenDropPoint)
{
	Tree::NodeHandle dropItem = GetHitItem (screenDropPoint);
	if (!dropItem.IsNull () && dropItem != _previousDragOverItem)
	{
		ClearDropTargetItem ();
		_view.SetHilite (dropItem);
		_previousDragOverItem = dropItem;
	}
}

void HierarchyController::ClearDropTargetItem ()
{
	if (!_previousDragOverItem.IsNull ())
	{
		_view.RemoveHilite (_previousDragOverItem);
		_previousDragOverItem.Reset ();
	}
}

void HierarchyController::ChangeCurFolder (Tree::NodeHandle node)
{
	Assert (_browser != 0 && _listObserver != 0);
	if (_browser->IsCurrentDir (node))
		return;
	File::Vpath relPath;
	_browser->OnSelect(node, relPath);
	// Revisit: if selected folder doesn't exists the list view doesn't change but the tree
	// view still shows the non-existing folder
	NamedValues values;
	values.Add ("FolderName", relPath.ToString ());
	_executor.PostCommand ("SetCurrentFolder", values);
}

bool HierarchyController::OnGetDispInfo (Tree::View::Request const & request,
										 Tree::View::State const & state,
										 Tree::View::Item & item) throw ()
{
#if 0
	Tree::NodeHandle node = item.GetNode ();
	return true;
	if (request.IsText () && item.GetTextLen () > 0)
	{
		_browser->CopyField (node, item.GetBuf (), item.GetTextLen ());
		item.SetText ();
	}
	if (request.IsIcon ())
	{
		int image = 0, selImage = 0;
		_browser->GetImage (node, image, selImage);
		item.SetIcon (image, selImage);
	}
	if (request.IsChildCount ())
	{
		int count = _browser->GetChildCount (node);
		item.SetChildCount (count);
	}
#endif
	return true;
}

bool HierarchyController::OnItemExpanded (Tree::View::Item & item,
										  Tree::Notify::Action action) throw ()
{
	return true;
}

bool HierarchyController::OnItemCollapsing (Tree::View::Item & item,
											Tree::Notify::Action action,
											bool & allow) throw ()
{
	allow = true;
	return true;
}

bool HierarchyController::OnSelChanging (Tree::View::Item & itemOld, 
										 Tree::View::Item & itemNew, 
										 Tree::Notify::Action action) throw ()
{
	return true;
}

//-----------------
// Table Controller
//-----------------

TableController::TableController (int id,
								  Win::Dow::Handle win, 
								  Cmd::Executor & executor, 
								  Focus::Ring & focusRing,
								  bool isDragSource,
								  bool isSingleSelection)
	: Notify::ListViewHandler (id),
	  _id (id),
	  _executor (executor),
	  _focusRing (focusRing),
	  _kbdHandler (executor, focusRing),
	  _newItem (false),
	  _view (win, id, isSingleSelection),
	  _browser (0),
	  _listObserver (0),
	  _boldFont (true, false),
	  _italicFont (false, true),
	  _isDragSource(isDragSource)
{}

bool TableController::ItemKbdHandler::OnTab () throw ()
{
	if (IsShift ())
		_focusRing.SwitchToPrevious ();
	else
		_focusRing.SwitchToNext ();
	return true;
}

bool TableController::ItemKbdHandler::OnReturn () throw ()
{
	_executor.ExecuteCommand ("Selection_Open");
	return true;
}

bool TableController::IsInsideView (Win::Point const & screenPt) const
{
	Win::ClientRect clientRect (_view);
	Win::Point clientPt (screenPt);
	_view.ScreenToClient (clientPt);
	return clientRect.IsInside (clientPt);
}

bool TableController::OnSetFocus (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
{
	_focusRing.SwitchToThis (idFrom);
	return true;
}

bool TableController::OnDblClick () throw ()
{
	NamedValues dummy;
	_executor.PostCommand ("Selection_Open", dummy);
	return true;
}

bool TableController::OnGetDispInfo (Win::ListView::Request const & request,
									 Win::ListView::State const & state,
									 Win::ListView::Item & item) throw ()
{
	Assert (_browser != 0);
	int itemIdx = item.GetIdx ();
	if (request.IsText ())
	{
		_browser->CopyField (itemIdx, item.GetSubItem (), item.GetBuf (), item.GetTextLen ());
		item.SetText ();
	}
	if (item.GetSubItem () == 0) // leftmost column
	{
		if (request.IsIcon ())
		{
			int iOverlay = 0;
			int iIcon = 0;
			_browser->GetImage (itemIdx, iIcon, iOverlay);
			item.SetIcon (iIcon);
			item.SetOverlay (iOverlay);
		}
	}
	return true;
}

bool TableController::OnItemChanged (Win::ListView::ItemState & state) throw ()
{
	Assert (_browser != 0 && _listObserver != 0);
	if (state.GainedSelection () || state.LostSelection ())
	{
		_browser->SelChanged (state.Idx (), state.WasSelected (), state.IsSelected ());	
		_listObserver->OnItemChange ();
	}
	return true;
}

bool TableController::OnBeginLabelEdit (long & result) throw ()
{
	// If not new item creation check if rename command is available
	// Revisit: ask the browser for the command name to check
	if (!_newItem && !_executor.IsEnabled ("Selection_Rename"))
	{
		result = TRUE;	// Don't allow label editing
	}
	else
	{
		// We allow label editing -- disable keyboard accelerators
		result = FALSE;
		_executor.DisableKeyboardAccelerators (0);
	}
	return true;
}

bool TableController::OnEndLabelEdit (Win::ListView::Item * item, long & result) throw ()
{
	Assert (_browser != 0);
	// Enable keyboard accelerators
	_executor.EnableKeyboardAccelerators ();
	if (item->GetBuf () != 0)
	{
		TrimmedString newName (item->GetBuf ());
		ThePrompter.SetNamedValue ("NewName", newName);
		// Revisit: ask the browser for the command name to execute
		if (_newItem)
			_executor.ExecuteCommand ("DoNewFolder");
		else
			_executor.ExecuteCommand ("DoRenameFile");
		ThePrompter.ClearNamedValues ();
		result = 1;
	}
	else
	{
		// User aborted label edit
		result = 0;
		if (_newItem)
			_browser->AbortNewItemEdit ();
	}
	_newItem = false;
	return true;
}

bool TableController::OnColumnClick (int col) throw ()
{
	Assert (_browser != 0);
	_browser->Resort (col);
	return true;
}

void TableController::OnBeginDrag (Win::Dow::Handle winFrom, unsigned idFrom, int itemIdx, bool isRightButtonDrag) throw ()
{
	if (!_isDragSource)
		return;
	std::string button ((isRightButtonDrag ? "right" : "left"));
	ThePrompter.SetNamedValue("Button", button);
	ThePrompter.SetNamedValue("WindowFrom", winFrom);
	ThePrompter.SetNamedValue("IdFrom", idFrom);
	_executor.ExecuteCommand ("DoDrag");
	ThePrompter.ClearNamedValues ();
}

void TableController::OnCustomDraw (Win::ListView::CustomDraw & customDraw, long & result) throw ()
{
	result = Win::ListView::CustomDraw::DoDefault;	// Assume default drawing
	RecordSet const * recordSet = _browser->GetRecordSet ();
	if (recordSet != 0 && recordSet->SupportsDrawStyles ())
	{
		if (customDraw.IsPrePaint ())
		{
			result = Win::ListView::CustomDraw::NotifyItemDraw;
		}
		else if (customDraw.IsItemPrePaint ())
		{
			unsigned row = _browser->GetRow (customDraw.GetItemIdx ());
			DrawStyle style = recordSet->GetStyle (row);
			if (style != DrawNormal)
			{
				result = Win::ListView::CustomDraw::NewFont;
				switch (style)
				{
				case DrawGreyed:
					customDraw.SetTextColor (Win::ColorText::Greyed ());
					_italicFont.SelectFont (customDraw);
					break;
				
				case DrawBold:
					_boldFont.SelectFont (customDraw);
					break;
				
				case DrawHilite:
					customDraw.SetTextColor (Win::ColorText::Red ());
					_italicFont.SelectFont (customDraw);
					break;
				}
			}
		}
	}
}

void TableController::FontHandler::SelectFont (Win::ListView::CustomDraw & customDraw)
{
	Win::Canvas itemCanvas = customDraw.GetItemCanvas ();
	if (_font.IsNull ())
	{
		// Create font from the current font
		Font::Descriptor oldFont (itemCanvas);
		Font::Maker newFontMaker (oldFont);
		if (_isItalic)
			newFontMaker.SetItalic (true);
		if (_isBold)
			newFontMaker.MakeBold ();
		_font = newFontMaker.Create ();
	}
	itemCanvas.SelectObject (Gdi::Handle (_font.ToNative ()));
}

//---------------------------
// File Drop Table Controller
//---------------------------

FileDropTableController::FileDropTableController (int id,
												  Win::Dow::Handle win,
												  Cmd::Executor & executor,
												  Focus::Ring & focusRing,
												  bool isDragSource,
												  bool isSingleSelection)
	: TableController (id, win, executor, focusRing, isDragSource, isSingleSelection),
	  _previousDragOverItem (-1)
{
	RegisterAsDropTarget(_view);	// Register controller's window as file drop target window
}

void FileDropTableController::OnDragEnter (Win::Point screenDropPoint)
{
	SetDropTargetItem (screenDropPoint);
}

void FileDropTableController::OnDragLeave ()
{
	ClearDropTargetItem ();
}

void FileDropTableController::OnDragOver (Win::Point screenDropPoint)
{
	SetDropTargetItem (screenDropPoint);
}

void FileDropTableController::OnFileDrop (Win::FileDropHandle droppedFiles, Win::Point screenDropPoint)
{
	ClearDropTargetItem ();
	NamedValues args;
	int itemIdx = GetHitItem (screenDropPoint);
	if (itemIdx != -1)
	{
		std::string hitItemString = _view.RetrieveItemText (itemIdx);
		args.Add ("DropTargetString", hitItemString);
	}
	else
	{
		args.Add ("DropTargetString", "");
	}
	args.Add ("FileDropHandle", ::ToHexString (reinterpret_cast<unsigned int>(droppedFiles.ToNative ())));
	_executor.PostCommand ("DoDrop", args);
}

int FileDropTableController::GetHitItem (Win::Point screenDropPoint)
{
	Win::Point viewDropPoint (screenDropPoint);
	_view.ScreenToClient (viewDropPoint);
	return _view.GetHitItem (viewDropPoint);
}

void FileDropTableController::SetDropTargetItem (Win::Point screenDropPoint)
{
	int itemIdx = GetHitItem (screenDropPoint);
	if (itemIdx != _previousDragOverItem)
	{
		ClearDropTargetItem ();
		if (itemIdx != -1)
		{
			RecordSet const * recordSet = _browser->GetRecordSet ();
			int row = _browser->GetRow (itemIdx);
			FileType type (recordSet->GetType (row));
			if (type.IsFolder ())
			{
				_view.SetHilite (itemIdx);
				_previousDragOverItem = itemIdx;
			}
		}
	}
}

void FileDropTableController::ClearDropTargetItem ()
{
	if (_previousDragOverItem != -1)
	{
		_view.RemoveHilite (_previousDragOverItem);
		_previousDragOverItem = -1;
	}
}


//-----------------------
// Range Table Controller
//-----------------------

void RangeTableController::ShowView ()
{
	TableController::ShowView ();
	if (_browser->SelCount () == 0)
	{
		// Force 'CreateRange' command when no selection.
		// No selected items, so no item change notification can trigger
		// the 'CreateRange' command
		// dbg << "     Posting command 'CreateRange' in the empty range table" << std::endl;
		NamedValues args;
		args.Add ("Extend", "no");
		_executor.PostCommand ("CreateRange", args);
	}
}

bool RangeTableController::OnItemChanged (Win::ListView::ItemState & state) throw ()
{
	// dbg << "--> RangeTableController::OnItemChanged" << std::endl;
	Assert (_browser != 0 && _listObserver != 0);
	if (state.GainedSelection () || state.LostSelection ())
	{
		_browser->SelChanged (state.Idx (), state.WasSelected (), state.IsSelected ());	
		_listObserver->OnItemChange ();
		if (state.GainedSelection ())
		{
			Assert (_browser->SelCount () != 0);
			dbg << "     Posting command 'CreateRange'" << std::endl;
			NamedValues args;
			args.Add ("Extend", "no");
			_executor.PostCommand ("CreateRange", args);
		}
	}
	// dbg << "<-- RangeTableController::OnItemChanged" << std::endl;
	return true;
}

//-----------------
// Tab Controller
//-----------------

TabController::TabController (Win::Dow::Handle win, UiManager * uiMan)
	: Notify::TabHandler (ID_VIEWTABS),
	  _uiMan (uiMan),
	  _view (win, ID_VIEWTABS)
{}

bool TabController::OnSelChange () throw ()
{
	_uiMan->TabChanged ();
	return true;
}

//---------------------
// Drop Down Controller
//---------------------

void DropDownCtrl::Refresh (std::string const & items)
{
	// Items can be a multi-line text.  Break it into
	// separate lines, display first line and add the rest
	// to the drop down list.
	unsigned int eolPos = items.find_first_of ("\r\n");
	if (eolPos != std::string::npos)
	{
		std::string firstLine (&items [0], eolPos);
		_dropDown.Display (firstLine.c_str ());
		do
		{
			unsigned int startPos = eolPos;
			while (IsEndOfLine (items [startPos]))
				startPos++;
			eolPos = items.find_first_of ("\r\n", startPos);
			int lineLen = (eolPos == std::string::npos) 
				? items.length () - startPos 
				: eolPos - startPos;
			std::string line (&items [startPos], lineLen);
			_dropDown.AddToList (line.c_str ());
		} while (eolPos != std::string::npos);
	}
	else
	{
		// Single item
		_dropDown.Display (items.c_str ());
	}
	_dropDown.Show ();
}

//---------------------
// History Filter Controller
//---------------------

HistoryFilterCtrl::HistoryFilterCtrl (int id,
									  Win::Dow::Handle topWin,
									  Win::Dow::Handle canvasWin,
									  Cmd::Executor & executor)
	: DropDownCtrl (id, topWin, canvasWin),
	  _executor (executor)
{
}

bool HistoryFilterCtrl::OnControl (unsigned id, unsigned notifyCode) throw (Win::Exception)
{
	// Notification from the drop down on the tool bar
	if (Win::DropDown::GotFocus (notifyCode))
		_executor.DisableKeyboardAccelerators (0);
	else if (Win::DropDown::LostFocus (notifyCode))
		_executor.EnableKeyboardAccelerators ();

	bool isSelection;
	if (_dropDown.OnNotify (notifyCode, isSelection))
	{
		std::string	newFilter = _dropDown.RetrieveTrimmedEditText ();
		ThePrompter.SetNamedValue ("Filter", newFilter);
		_executor.ExecuteCommand ("ChangeFilter");
		ThePrompter.ClearNamedValues ();
	}
	return true;
}

//---------------------
// Target Project Controller
//---------------------

TargetProjectCtrl::TargetProjectCtrl (int id,
									  Win::Dow::Handle topWin,
									  Win::Dow::Handle canvasWin,
									  Cmd::Executor & executor)
	: DropDownCtrl (id, topWin, canvasWin),
	  _executor (executor)
{
}

bool TargetProjectCtrl::OnControl (unsigned id, unsigned notifyCode) throw (Win::Exception)
{
	// Notification from the drop down on the tool bar
	if (Win::DropDown::GotFocus (notifyCode))
		_executor.DisableKeyboardAccelerators (0);
	else if (Win::DropDown::LostFocus (notifyCode))
		_executor.EnableKeyboardAccelerators ();

	bool isSelection;
	if (_dropDown.OnNotify (notifyCode, isSelection))
	{
		std::string	targetProject = _dropDown.RetrieveTrimmedEditText ();
		if (targetProject == NoTarget)
			return true;

		if (targetProject == SelectBranch)
			targetProject.clear ();

		ThePrompter.SetNamedValue ("Target", targetProject);
		_executor.ExecuteCommand ("Project_SelectMergeTarget");
		ThePrompter.ClearNamedValues ();
	}
	return true;
}

//---------------------
// Merge Type Controller
//---------------------

MergeTypeCtrl::MergeTypeCtrl (int id,
							  Win::Dow::Handle topWin,
							  Win::Dow::Handle canvasWin,
							  Cmd::Executor & executor)
	: Control::Handler (id),
	  _executor (executor)
{
	Win::ComboBoxMaker comboMaker (topWin, id, Win::ComboBox::Style::DropDownList);
	comboMaker.Style () << Win::Style::Ex::ClientEdge;
	_dropDownList.Reset (comboMaker.Create (canvasWin));
}

void MergeTypeCtrl::Refresh (MergeTypeEntry const items [])
{
	unsigned idx = 0;
	while (items [idx]._dispName != 0)
	{
		_dropDownList.AddToList (items [idx]._dispName);
		++idx;
	}
	_dropDownList.Show ();
}

void MergeTypeCtrl::Select (std::string const & item)
{
	_dropDownList.SelectString (item.c_str ());
}

bool MergeTypeCtrl::OnControl (unsigned id, unsigned notifyCode) throw (Win::Exception)
{
	// Notification from the drop down list on the tool bar
	if (notifyCode == CBN_SELENDOK)
	{
		std::string	mergeType = _dropDownList.RetrieveTrimmedEditText ();
		unsigned i = 0;
		while (MergeTypeEntries [i]._dispName != 0)
		{
			if (MergeTypeEntries [i]._dispName == mergeType)
				break;
			++i;
		}
		Assert (MergeTypeEntries [i]._dispName != 0);

		ThePrompter.SetNamedValue ("AncestorMerge", MergeTypeEntries [i]._isAncestor? "yes": "no");
		ThePrompter.SetNamedValue ("CumulativeMerge", MergeTypeEntries [i]._isCumulative? "yes": "no");

		_executor.ExecuteCommand ("Project_SetMergeType");
		ThePrompter.ClearNamedValues ();
	}
	return true;
}

//---------------------
// Read-only Display Controller
//---------------------

InfoDisplayCtrl::InfoDisplayCtrl (int id,
								  Win::Dow::Handle topWin,
								  Win::Dow::Handle canvasWin,
								  Cmd::Executor & executor)
	: Control::Handler (id),
	  _display (id, topWin, canvasWin),
	  _executor (executor)
{
}

bool InfoDisplayCtrl::OnControl (unsigned id, unsigned notifyCode) throw (Win::Exception)
{
	// Notification from the info display on the tool bar
	if (Win::ReadOnlyDisplay::GotFocus (notifyCode))
		_executor.DisableKeyboardAccelerators (0);
	else if (Win::ReadOnlyDisplay::LostFocus (notifyCode))
		_executor.EnableKeyboardAccelerators ();
	return true;
}
