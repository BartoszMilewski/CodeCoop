//
// (c) Reliable Software, 1997-2006
//
#include "precompiled.h"
#include "EditPane.h"
#include "resource.h"
#include "ClipboardText.h"
#include "EditParams.h"
#include "OutputSink.h"
#include "Search.h"
#include "Parser.h"
#include "UndoSink.h"
#include "Registry.h"

#include <Ctrl/MarginCtrl.h>
#include <Ex/WinEx.h>
#include <LightString.h>
#include <Win/Keyboard.h>

class LoggingAction
{
public:
	LoggingAction (EditLog::Action::Bits action, DocPosition & docPosBeg, Selection::Marker & selection, EditLog & undoLog, EditLog & redoLog)
		:_undoLog (undoLog), _redoLog (redoLog)
	{
		_undoLog.BeginAction (action, docPosBeg, selection.IsValid ());
		if (selection.IsValid ())
			_undoLog.OldSelection (selection);
	}
	void Commit (DocPosition const & docPosEnd, Selection::Marker selection = Selection::Marker ())
	{ 
		_docPosEnd = docPosEnd;
		if (selection.IsValid ())
			_undoLog.NewSelection (selection);
		_undoLog.EndAction (_docPosEnd);
	} 
	~LoggingAction ()
	{		
		if (!_docPosEnd.IsValid ()) 
		{
			_undoLog.Clear ();
			_redoLog.Clear ();
		}
	}
private:
	DocPosition			_docPosEnd;
	EditLog &			_undoLog;
	EditLog &			_redoLog;
};

bool KbdHandler::OnPageUp () throw ()
{
	_ctrl->PageUp (IsShift ());
	return true;
}

bool KbdHandler::OnPageDown () throw ()
{
	_ctrl->PageDown (IsShift ());
	return true;
}

bool KbdHandler::OnUp () throw ()
{
	_ctrl->Up (IsShift (), IsCtrl ());
	return true;
}

bool KbdHandler::OnDown () throw ()
{
	_ctrl->Down (IsShift (), IsCtrl ());
	return true;
}

bool KbdHandler::OnLeft () throw ()
{
	_ctrl->Left (IsShift (), IsCtrl ());
	return true;
}

bool KbdHandler::OnRight () throw ()
{
	_ctrl->Right (IsShift (), IsCtrl ());
	return true;
}

bool KbdHandler::OnHome () throw ()
{
	_ctrl->Home (IsShift (), IsCtrl ());
	return true;
}

bool KbdHandler::OnEnd () throw ()
{
	_ctrl->End (IsShift (), IsCtrl ());
	return true;
}

bool KbdHandler::OnDelete () throw ()
{
	_ctrl->Delete ();
	return true;
}

void EditStateNotificationSink::ForwardAction () 
{
	++_actionCounter;
	// transition -1 -> 0 : disable Save
	// transition 0 -> 1 : enable Save
	if (_actionCounter == 0 || _actionCounter == 1)
		OnChangeState ();	 
}

void EditStateNotificationSink::BackwardAction () 
{
	--_actionCounter;
	// transition 1 -> 0 : disable Save
	// transition 0 -> -1 : enable Save
	if (_actionCounter == 0 || _actionCounter == -1)
		OnChangeState (); 
}

void EditStateNotificationSink::OnSavedDoc ()
{
	_actionCounter = 0;
	OnChangeState ();
}

EditPaneController::EditPaneController ()
	: _hwndParent (0),
#pragma warning (disable:4355)
	  _kbdHandler (this),
	  _redoLog (this),
	  _undoLog (this, _redoLog),
#pragma warning (default:4355)	  
	  _timer (2),
	  _isReadOnly (true),	  
	  _doc (&_undoLog)
{} 

EditPaneController::~EditPaneController ()
{} 

bool EditPaneController::OnCreate (Win::CreateData const * create, bool & success) throw ()
{
	try
	{
		_hwndParent = create->GetParent ();
		Registry::UserDifferPrefs prefs;
		bool useTabs = prefs.GetUseTabs();
		unsigned tabSizeChar = prefs.GetTabSize ();
		if (tabSizeChar == 0)
			tabSizeChar = 4;	// default: tab == 4 spaces
		_fontBox.reset (new FontBox (_h, useTabs, tabSizeChar));
		_mapper.reset (new ColMapper (_h, _fontBox->GetFont ()));
		_caret.reset (new Caret (_h, *_mapper));
		_timer.Attach (_h);
		// pass 'this' as notification sink
		_viewPort.reset (new ViewPort (this , _fontBox->GetTabSizeChar ()));
		_doc.SetParaNotificationSink (_viewPort.get ());// pass viewport as notification sink
		_selection.SetSelectionNotificationSink (_viewPort.get ());// pass viewport as notification sink
		success = true;
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Display ("Window Initialization -- Unknown Error", Out::Error); 
	}
	return true;
}

bool EditPaneController::OnFocus (Win::Dow::Handle winPrev) throw ()
{
	_caret->Create ();
	SetCaret ();
	return true;
}

bool EditPaneController::OnKillFocus (Win::Dow::Handle winNext) throw ()
{
	_caret->Kill ();
	return true;
}

bool EditPaneController::OnSize (int width, int height, int flag) throw ()
{
	_cx = width; 
	_cy = height;
	int cVisCol = _mapper->XtoColRound (_cx);
	int cVisLine = _mapper->YtoLineRound (_cy);
	 _viewPort->SetSize (cVisCol, cVisLine, _doc);
	if (_h.HasFocus ())
		SetCaret ();
	return true;
}

bool EditPaneController::OnCommand (int cmdId, bool isAccel) throw ()
{
	// Commands can get here from accelerators.
	// Top level window will deal with them the same way
	// it deals with menu commands.
	_hwndParent.SendMsg (WM_COMMAND, static_cast<WPARAM>(cmdId));
	return true;
}

bool EditPaneController::OnMouseMove (int x, int y, Win::KeyState kState) throw ()
{
	if (kState.IsLButton ())
	{
		LButtonDrag (x, y);
		return true;
	}
	return false;
}

bool EditPaneController::OnUserMessage (Win::UserMessage & msg) throw ()
{
	switch (msg.GetMsg ())
	{
	case UM_INITDOC:
		{
			std::unique_ptr<EditBuf> * bufPtr = 
				reinterpret_cast<std::unique_ptr<EditBuf> *> (msg.GetLParam ());
			InitDoc (std::move(*bufPtr));
			return true;
		}
	case UM_GETDOCDIMENSION:
		msg.SetResult (GetDocDimension (msg.GetWParam ()));
		return true;
	case UM_GETWINDIMENSION:
		msg.SetResult (GetWinDimension (msg.GetWParam ()));
		return true;
	case UM_SETDOCPOSITION:
		SetDocPosition (msg.GetWParam (), msg.GetLParam (), true);
		return true;;
	case UM_GETDOCPOSITION:
		msg.SetResult (GetDocPosition (msg.GetWParam ()));
		return true;
	case UM_SELECT_ALL:
		SelectAll ();
		return true;
	case UM_SYNCH_SCROLL:
		SynchScrollToDocPos (msg.GetWParam (), msg.GetLParam ());
		return true;
	case UM_CHANGE_FONT:
		ChangeFont (msg.GetWParam (), reinterpret_cast<Font::Maker *> (msg.GetLParam ()));
		return true;
	case UM_STARTSELECTLINE:
		StartSelectLine (msg.GetWParam (), msg.GetLParam ());
		return true;
	case UM_SELECTLINE:
		ContinueSelectLine (msg.GetLParam ());
		return true;
	case UM_ENDSELECTLINE:
		EndSelectLine (msg.GetLParam ());
		return true;
	case UM_SEARCH_NEXT:
		NextChange ();
		return true;
	case UM_SEARCH_PREV:
		PrevChange ();
		return true;
	case UM_EDIT_COPY:
		CopySelectionToClipboard ();
		return true;
	case UM_EDIT_PASTE:
		msg.SetResult (Paste ());
		return true;
	case UM_EDIT_CUT:
		Cut ();
		return true;
	case UM_EDIT_DELETE:
		Delete ();
		return true;
	case UM_EDIT_NEEDS_SAVE:
		msg.SetResult (DoesNeedSave (reinterpret_cast<unsigned int *>(msg.GetLParam ()), msg.GetWParam () == 1));
		return true;
	case UM_EDIT_IS_READONLY:
		msg.SetResult (IsReadOnly ());
		return true;
	case UM_SET_EDIT_READONLY:
		ChangeReadOnlyState (msg.GetWParam () != 0);
		return true;
	case UM_GET_BUF:
		msg.SetResult (GetBuf (reinterpret_cast<char *>(msg.GetLParam ()), msg.GetWParam () == 0));
		return true;
	case UM_GET_SELECTION:
		GetSelection (reinterpret_cast<SearchRequest *>(msg.GetLParam ()));
		return true;
	case UM_FIND_NEXT:
		FindNext (reinterpret_cast<SearchRequest const *>(msg.GetLParam ()));
		return true;
	case  UM_TOGGLE_LINE_BREAKING:
		ToggleLineBreaking (msg.GetWParam () != 0);
		return true;
	case UM_GETSCROLLPOSITION:
		msg.SetResult (GetScrollPosition (msg.GetWParam ()));
		return true;
	case UM_CLEAR_EDIT_STATE:
		ClearEditState ();
		return true;
	case UM_REPLACE:
		Replace (reinterpret_cast<ReplaceRequest const *>(msg.GetLParam ()));
		return true;
	case UM_REPLACE_ALL:
		ReplaceAll (reinterpret_cast<ReplaceRequest const *>(msg.GetLParam ()));
		return true;
	case UM_GET_DOC_SIZE:
		msg.SetResult (_doc.GetParagraphCount ());
		return true;
	case UM_GOTO_PARAGRAPH:
		GoToParagraph (msg.GetLParam ());
		return true;
	case UM_CAN_UNDO:
		msg.SetResult (!_undoLog.Empty ());
		return true;
	case UM_UNDO:
		Undo (msg.GetLParam ());
		return true;
	case UM_CAN_REDO:
		msg.SetResult (!_redoLog.Empty ());
		return true;
	case UM_REDO:
		Redo (msg.GetLParam ());
		return true;
	}
	return false;
}

void EditPaneController::InitDoc (std::unique_ptr<EditBuf> editBuf)  
{
	try
	{
		_isReadOnly = true;
		_undoLog.Clear ();
		_redoLog.Clear ();
		_actionCounter = 0;
		_doc.Init (std::move(editBuf));
		_viewPort->NewDoc (_doc);
		int count = _doc.GetParagraphCount (); 
		for (int i = 0; i < count; i++) 
		{ 
			_viewPort->UpdateMaxParagraphLen (_doc.GetParagraph (i)); 
		}
		_selection.Clear ();
		DocPosNotify ();
	}
	catch (...)
	{
		Win::ClearError ();
		// Revisit: API
		::MessageBox (_h.ToNative (), "Error initializing edit pane", "Editor", MB_OK);
	}
} 

bool EditPaneController::OnTimer (int id) throw ()
{ 
	_timer.Kill (); 
	if (_h.HasCapture ()) 
	{ 
		LButtonDrag (_mousePoint.x, _mousePoint.y); 
	}
	return true;
} 

void EditPaneController::NewParagraph ()
{
	DocPosition docPosition = _viewPort->GetDocPosition ();
	if (!CanEdit ()) 
	{ 
		ClearSelection ();
		docPosition.SetParaOffset (0);
		docPosition.IncreaseParaNo ();
		SetDocPositionShow (docPosition);	
		DocPosNotify ();
		return;
	} 
	else 
	{
		LoggingAction xLog (EditLog::Action::newLine, 
							docPosition, 
							Selection::Marker (_selection), 
							_undoLog, 
							_redoLog);
		if (!_selection.IsEmpty ())
		{
			DeleteSelection ();
			docPosition = _viewPort->GetDocPosition (); 
		} 

		if (docPosition.ParaNo () > _doc.GetDocEndParaPos ()) 
		{ 
			_doc.AppendParagraph (docPosition.ParaNo () + 1); 
		} 
		else 
		{ 
			_doc.SplitParagraph (docPosition);
		}
		docPosition.IncreaseParaNo ();
		docPosition.SetParaOffset (0);
		xLog.Commit (docPosition);		
	}
	int docPara = docPosition.ParaNo ();
	docPosition.SetParaOffset (AutoIndent (docPara ));
	SetDocPositionShow (docPosition);	
	DocPosNotify ();
}

// Keyboard
bool EditPaneController::OnChar (int vKey, int flags) throw ()
{ 
	if (!CanEdit ())
	{
		ClearSelection (); 
		return true;
	}
	if (vKey == VKey::Return)
	{
		NewParagraph ();
		return true;
	}
	else if (vKey == VKey::BackSpace)
	{
		BackSpace ();
		return true;
	}
	else if (vKey == VKey::Escape)
	{
		Escape ();
		return true;
	}
	else if (vKey == VKey::Tab)
	{
		if (MultiParaTab (_kbdHandler.IsShift ()))
			return true;
	}
	 
	DocPosition docPosition = _viewPort->GetDocPosition ();
	LoggingAction xLog (EditLog::Action::continueTyping, 
						docPosition, 
						Selection::Marker (_selection), 
						_undoLog, 
						_redoLog);
	if (!_selection.IsEmpty ())
	{ 
		DeleteSelection ();
		docPosition = _viewPort->GetDocPosition (); 
	} 

	_doc.InsertChar (docPosition, vKey);
	docPosition.IncraseParaOffset ();
	xLog.Commit (docPosition);
	SetDocPositionShow (docPosition); 
	DocPosNotify ();
	return true;
} 

int EditPaneController::AutoIndent (int paraNo)
{
	Assert (paraNo > 0);
	EditParagraph * para = _doc.GetParagraph (paraNo - 1);
	char const * buf = para->Buf ();
	int len = para->Len ();
	int tabs = 0;
	for (int k = 0; k < len; )
	{
		if (buf [k] == '\t')
		{
			++tabs;
			++k;
		}
		else if (para->IsSpace (k))
		{
			int tabsize = _fontBox->GetTabSizeChar ();
			int numberSpace = 0;
			while (k < len && para->IsSpace (k) && numberSpace < tabsize)
			{
				++k;
				++numberSpace;
			}
			if (numberSpace == tabsize)
				++tabs;
			else if (k < len && buf [k] == '\t')
			{
				++tabs;
				++k;
			}
		}
		else
			break;
	}
	
	int charCount = 0;

	if (tabs > 0)
	{
		DocPosition insertPos = DocPosition (paraNo, 0);
		LoggingAction xLog (EditLog::Action::autoIndent, 
							insertPos, 
							Selection::Marker (_selection),
							_undoLog, 
							_redoLog);
		std::string tbs;
		if (_fontBox->GetUseTabs())
		{
			charCount = tabs;
			tbs = std::string(tabs, '\t');
		}
		else
		{
			charCount = tabs * _fontBox->GetTabSizeChar();
			tbs = std::string(charCount, ' ');
		}
		_doc.InsertBuf (insertPos, tbs.c_str (), tbs.size ());
		xLog.Commit (DocPosition (paraNo, charCount));
	}
	return charCount;
}

void EditPaneController::Delete ()
{
	if (!CanEdit ()) 
		return; 

	DocPosition docPosition = _viewPort->GetDocPosition ();
	LoggingAction xLog (EditLog::Action::del, 
						docPosition, 
						Selection::Marker (_selection), 
						_undoLog, 
						_redoLog);
	if (_selection.IsEmpty ())
	{
		if (docPosition.ParaOffset () >= _doc.GetParagraphLen (docPosition.ParaNo ())) 
		{ 
			if (docPosition.ParaNo () == _doc.GetDocEndParaPos ()) 
			{
				xLog.Commit (docPosition);
				return;
			}

			// Revisit later: fill virtual space  
			// after line end with spaces (or tabs) 
			// update maxLineLen 

			MergeParagraphs (docPosition.ParaNo ()); 
		} 
		else
		{
			// Notice: don't care if longest line  
			// was shortened
			_doc.DeleteChar (docPosition);
		}
	}
	else
	{
		DeleteSelection ();
	}
	xLog.Commit (_viewPort->GetDocPosition ());
}

void EditPaneController::Escape ()
{
	ClearSelection ();
}

bool EditPaneController::CanEdit ()
{
	if (_isReadOnly)
	{
		// Ask parent if we can switch to edit mode
		Win::UserMessage msg (UM_CHECK_OUT);
		_hwndParent.SendMsg (msg);
		_isReadOnly = (msg.GetResult () == 0);
	}
	return !_isReadOnly;
}

void EditPaneController::BackSpace () 
{ 																		
	if (!CanEdit ())
		return;
	
	DocPosition docPosition = _viewPort->GetDocPosition ();
	LoggingAction xLog (EditLog::Action::backSpace, 
						docPosition, 
						Selection::Marker (_selection), 
						_undoLog, 
						_redoLog);
	if (_selection.IsEmpty ()) 
	{  
		if (docPosition.ParaOffset () == 0) 
		{
			if (docPosition.ParaNo () == 0)
			{
				xLog.Commit (docPosition);
				return; 
			}
			MergeParagraphs (docPosition.ParaNo () - 1);
		}
		else
		{
			docPosition.DecraseParaOffset ();
			_doc.DeleteChar (docPosition);			
			SetDocPositionShow (docPosition);
		} 
	} 
	else 
	{
		DeleteSelection ();
	}
	xLog.Commit (_viewPort->GetDocPosition ());
	DocPosNotify ();
} 

void EditPaneController::DeleteSelection () 
{
	if (_selection.IsEmpty ())
		return;
	Selection::Marker selection (_selection);
	ClearSelection ();
	_doc.Delete (selection);
	SetDocPositionShow (selection.Start ());
} 

void EditPaneController::MergeParagraphs (int startParaNo)
{
	EditParagraph * para = _doc.GetParagraph (startParaNo);
	int startCol = para->Len ();
	_doc.MergeParagraphs (startParaNo);
	SetDocPositionShow (DocPosition (startParaNo, startCol)); 
}

int EditPaneController::GetDocDimension (int bar)
{
	if (bar == SB_VERT)
		return _viewPort->GetDocLines (_doc);
	else if (bar == SB_HORZ)
		return _viewPort->GetMaxParagraphLen ();
	return 0;
}

int EditPaneController::GetWinDimension (int bar)
{
	if (bar == SB_VERT)
		return _viewPort->Lines ();
	else if (bar == SB_HORZ)
		return _viewPort->GetVisCols ();
	return 0;
}

void EditPaneController::SelectAll ()
{
	int cPara = _doc.GetParagraphCount ();
	if (cPara == 0)
		return;
	cPara--;
	_selection.Clear ();
	_selection.StartSelectLine (0, PARA_START, PARA_END, 0);
	_selection.ContinueSelect (cPara, PARA_END);
	// set position at the end of documment
	int offset = _doc.GetParagraph (cPara)->Len ();
	SetDocPositionShow (DocPosition (cPara ,offset));
}

void EditPaneController::SetDocPosition (int bar, int pos, bool notify)
{
	if (bar == SB_VERT)
	{
		int docPara = _viewPort->LineToPara (pos);
		EditParagraph const * para = _doc.GetParagraph (docPara);
		int delta = _viewPort->SetScrollPosition (para, pos);
		int offset = _viewPort->GetOffset ();
		_h.Scroll (0, - _mapper->LineToY (delta));		
		if (notify)
		{
			// Notify the parent about the need to synchronize
			// scroll position in the other window 
			         
		   int paraNo = 0;
		   if (para != 0)
			   paraNo =  para->GetParagraphNo ();
			Win::UserMessage msg (UM_SCROLL_NOTIFY, offset, paraNo);
			_hwndParent.SendMsg (msg);
		}
	}
	else if (bar == SB_HORZ)
	{
		int delta = _viewPort->SetDocCol (pos);
		_h.Scroll (- _mapper->ColToX (delta), 0);
		int paraNo = _viewPort->GetParagraphNo ();
		EditParagraph const * para = _doc.GetParagraph (paraNo);
		int paraOffset  = _viewPort->VisibleParaOffset (para);
		_viewPort->SetDocParaOffset (paraOffset);		
	}
}

int EditPaneController::GetDocPosition (int bar)
{
	int docPos = 0;
	if (bar == SB_VERT)
		docPos = _viewPort->GetCurPara ();
	else if (bar == SB_HORZ)
		docPos = _viewPort->GetCurDocCol ();
	return docPos;
}

//
// Paint
//

bool EditPaneController::OnPaint () throw ()
{
	PaintContext canvas (_h, *_fontBox);
	const int paraCount = _doc.GetParagraphCount ();
    const int startWinLineNo = _mapper->YtoLineTrunc (canvas.Top ());
	const int lastWinLineNo = _mapper->YtoLineRound (canvas.Bottom ());
    const int xOrg = - _mapper->ColToX (_viewPort->GetColOffset ());

    for (int winLineNo = startWinLineNo; winLineNo <= lastWinLineNo; winLineNo++)
	{						
		int docPara = _viewPort->LineWinToPara (winLineNo);
		if (docPara >= paraCount)
			return true;
		
		EditParagraph * para = _doc.GetParagraph (docPara);
		int paraOffsetBegin;  
		int paraOffsetEnd;
		if (!_viewPort->GetLineBoundaries (winLineNo, para, paraOffsetBegin, paraOffsetEnd))
		{
			//if no line
			return true;
		}
			
		int yWinOff = _mapper->LineToY (winLineNo);
		if (paraOffsetBegin < para->Len ())
		{
			int xWinOff = 0;
			int colDocSelect, colDocSelectEnd;
			if (_selection.IsSelected (docPara, paraOffsetBegin, paraOffsetEnd ,colDocSelect, colDocSelectEnd))//
			{
				if (colDocSelect > paraOffsetBegin)// Draw the string preceding selection
					xWinOff += canvas.StyledTextOut (xOrg, 
													 xWinOff, 
													 yWinOff, 
													 para, 
													 paraOffsetBegin, 
													 colDocSelect - paraOffsetBegin);												
				if (colDocSelectEnd > colDocSelect)				
				// Draw the selected part in highlight
					xWinOff += canvas.HighTextOut (xOrg, 
												   xWinOff, 
												   yWinOff, 
												   para,
												   colDocSelect, 
												   colDocSelectEnd - colDocSelect); 
												    												    										
				if (paraOffsetEnd > colDocSelectEnd)
					// Draw the string following selection
					canvas.StyledTextOut (xOrg, 
										  xWinOff, 
										  yWinOff, 
										  para, 
										  colDocSelectEnd,
										  paraOffsetEnd - colDocSelectEnd); 
											   											
			}
			else if (paraOffsetEnd > paraOffsetBegin) 
			{
				// No selection: just draw the whole line
				canvas.StyledTextOut (xOrg, 0, yWinOff, para, paraOffsetBegin, paraOffsetEnd - paraOffsetBegin);
			}
		}
		else if (para->Len () == 0 && _viewPort->GetColOffset () == 0)
		{
			// empty line: display single space in appropriate style
			if (para->IsChanged ())
			{
				EditParagraph lineSpace (" ", 1, para->GetStyle (), -1);
				canvas.StyledTextOut (xOrg, 0, yWinOff, &lineSpace, 0, 1);
			}
		}
#if 0   // display target line numbers (only for testing)
		int xMargin = _cx - 25;
		char numBuf [6];
		wsprintf (numBuf, "%d", para->GetParagraphNo ());
		canvas.Text (xMargin, y, numBuf, strlen (numBuf));
#endif
	}
	return true;
}

void EditPaneController::CopySelectionToClipboard () 
{
	Clipboard clipboard (_h);
	if (_selection.IsEmpty ())
	{
		clipboard.Clear ();
		return;
	}

	std::string localBuf;
	CopySelectionToBuf (localBuf);
	
	try
	{
		clipboard.PutText (localBuf.c_str (), localBuf.size ());
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch ( ... )
	{
		Win::ClearError ();
		TheOutput.Display ("Unknown exception while openning clipboard.");
	}
}


bool EditPaneController::Paste ()
{
	// Get data from clipboard and insert at current position 
	// If there is selection, replace selection.
	if (!CanEdit ()) 
		return false;
	if (!Clipboard::IsFormatText ()) 
		return true;
	
	Clipboard clipboard (_h); 
	try
	{
		ClipboardText buf (clipboard);
		DocPosition docPosition = _viewPort->GetDocPosition ();	
		LoggingAction xLog (EditLog::Action::paste, 
							docPosition, 
							Selection::Marker (_selection), 
							_undoLog, 
							_redoLog);
		if (!_selection.IsEmpty ())
		{
			DeleteSelection ();
			docPosition = _viewPort->GetDocPosition (); 
		}
		EditParagraph * paraSplit = _doc.GetParagraph (docPosition.ParaNo ());
		int targetParaNo = paraSplit? paraSplit->GetParagraphNo (): 0;
		_doc.InsertParagraphs (docPosition, buf, targetParaNo);
		docPosition.IncreaseParaNo (buf.GetCount ()); 
		if (buf.IsNewParagraphAtTextEnd ())
		{
			docPosition.SetParaOffset (0); 
		} 
		else 
		{ 
			EditParagraph const * lastPara = buf.GetParagraph (buf.GetCount () - 1);
			if (buf.GetCount () == 1)
				docPosition.IncraseParaOffset (lastPara->Len ());
			else
				docPosition.SetParaOffset (lastPara->Len ());
			docPosition.DecraseParaNo (); 
		}
		xLog.Commit (docPosition);
		SetDocPositionShow (docPosition);
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch ( ... )
	{
		Win::ClearError ();
		TheOutput.Display ("Unknown exception while openning clipboard.");
	}
	// Sends UM_REFRESH_UI, that examines clipboard
	// This can be done only after closing clipboard used
	// by the paste command
	return true;
}

void EditPaneController::Cut () 
{ 
	if (!CanEdit ())
	{
		CopySelectionToClipboard ();
		return;
	}

	if (!_selection.IsEmpty ())
	{
		LoggingAction xLog (EditLog::Action::cut, 
							_viewPort->GetDocPosition (), 
							Selection::Marker (_selection), 
							_undoLog, 
							_redoLog);
		CopySelectionToClipboard ();
		DeleteSelection ();
		xLog.Commit (_viewPort->GetDocPosition ());
	}
}

void EditPaneController::ChangeFont (unsigned tabSize, Font::Maker * newFont)
{
	_viewPort->SetTabSize (tabSize);
	_fontBox->ChangeFont (_h, tabSize, newFont);
	if (newFont != 0)
	{
		_mapper->Init (_fontBox->GetCharWidth (), _fontBox->GetCharHeight ());
	}
	_viewPort->SetSize (_mapper->XtoColRound (_cx), _mapper->YtoLineRound (_cy), _doc);
	Win::UserMessage msg (UM_UPDATESCROLLBARS);
	_hwndParent.SendMsg (msg); 
	if (_h.HasFocus ())
	{ 
		_caret->Kill (); 
		_caret->Create ();
		ScrollToDocPos ();
	}
	_h.ForceRepaint ();
}

LRESULT EditPaneController::DoesNeedSave (unsigned int * newSize, bool getSizeAlways) const
{
	if (getSizeAlways || !IsSavedDoc ())
	{
		unsigned docSize = 0;
		for (int i = 0; i < _doc.GetParagraphCount (); i++)
		{
			// +2 for "\r\n"
			docSize += _doc.GetParagraphLen (i) + 2;
		}
		*newSize = docSize;
		return TRUE;
	}
	return FALSE;
}

LRESULT EditPaneController::IsReadOnly () const
{
	return _isReadOnly ? TRUE : FALSE;
}

LRESULT EditPaneController::GetBuf (char * buf, bool willBeSaved)
{
	unsigned writeIdx = 0;
	for (int i = 0; i < _doc.GetParagraphCount (); i++)
	{
		EditParagraph const * para = _doc.GetParagraph (i);
		memcpy (&buf [writeIdx], para->Buf (), para->Len ());
		writeIdx += para->Len ();
		buf [writeIdx] = '\r';
		buf [writeIdx + 1] = '\n'; 
		writeIdx += 2;
	}
	if (willBeSaved)
	{	// Refresh document status 
		OnSavedDoc ();
		// Document status and undoLog state  must be synchronized
		// Last action on the undoLog mey be typing (extended), close it
		_undoLog.Flush ();
	}
	return writeIdx;
}

void EditPaneController::OnChangeState ()
{
	Win::UserMessage msg (UM_REFRESH_UI);
	_hwndParent.SendMsg (msg);
}

void EditPaneController::GetSelection (SearchRequest * searchRequest)
{
	if (! _selection.IsEmpty ())
	{
		Selection::Marker selection (_selection);
		int firstParaNo =  selection.StartParaNo ();
		EditParagraph * firstPara = _doc.GetParagraph (firstParaNo);
		int  startCol, endCol;		
		_selection.IsSelected (firstParaNo, startCol, endCol);
		if (endCol == PARA_END)
			endCol = firstPara->Len ();
		
		char const * begin = firstPara ->BufAt (startCol);				
		searchRequest->SetFindWord (std::string(begin, begin + endCol - startCol));
	}
}

bool EditPaneController::IsSelectedForReplace (SearchRequest const * searchRequest)
{
	if (_selection.IsEmpty ())
		return false;
	Selection::Marker selection (_selection);
	if (!selection.IsSegment ())
		return false;
	
	int offsetBeg = selection.StartOffset ();
	int offsetEnd = selection.EndOffset ();	
	EditParagraph * para = _doc.GetParagraph (selection.StartParaNo ());
	if (offsetEnd == PARA_END)
		offsetEnd =  para->Len ();
	if (std::string (para->BufAt (offsetBeg), para->BufAt (offsetEnd)) != searchRequest->GetFindWord ())
		return false;
	return true;			
}

bool EditPaneController::FindNext (SearchRequest const * searchRequest)
{
	if (searchRequest->GetFindWord ().empty ())
		return false;   
	int paraNo = _viewPort->GetCurPara ();
	if (paraNo >= _doc.GetParagraphCount ())
		return false;
	int startOffset  = _viewPort->GetCurDocCol ();
	Selection::ParaSegment inOut (paraNo, startOffset); 	
	// start search
	bool succsess = false;
	if (_doc.Find (searchRequest, inOut))
	{   // if we've just found the current selection
		// we need to search further
		if (!EqualsSelection (inOut))
			succsess = true;
		else 
		{
			startOffset = inOut.ParaOffset ();
			if (searchRequest->IsDirectionForward ())
				inOut.SetParaOffset (startOffset + 1);
			else
				inOut.SetParaOffset (startOffset - 1);

			succsess = _doc.Find (searchRequest, inOut);
		}
	}
	if (! succsess)
	{ // try restart search
		paraNo = 0;
		if (searchRequest->IsDirectionBackward ()) 
			paraNo = std::max (0, _doc.GetParagraphCount () - 1);
		
		if ( _viewPort->GetCurPara () != paraNo)//possible dialog
		{
			startOffset = 0;
			std::string reStart ("begin");
			if (searchRequest->IsDirectionBackward ())
			{
				reStart = "end";
				startOffset = std::max (0, _doc.GetParagraphLen (paraNo) - 1);
			}
			Msg dialog;
			dialog << "Cannot find \"" << searchRequest->GetFindWord ().c_str () << "\" .";
			dialog << "\nContinue searching from the " << reStart.c_str() << " of the file?";			
			Out::Answer answer = TheOutput.PromptModal (dialog.c_str (), Out::YesNo);
			if (answer == Out::Yes)
			{
				inOut = Selection::ParaSegment (paraNo, startOffset);
				succsess = _doc.Find (searchRequest, inOut);
			}
			else //answer no
				return false;
		}
	} // end search
		
	if (!succsess)
	{
		std::string info ("Cannot find \"");
		info += searchRequest->GetFindWord ();
		info += "\".";
		TheOutput.DisplayModal (info.c_str ());
		return false;
	}

	ClearSelection ();
	paraNo = inOut.ParaNo ();
	startOffset = inOut.ParaOffset ();
	int endOffset = startOffset + inOut.Len ();
    _selection.CreateSelection (paraNo, startOffset, endOffset);	
	_viewPort->SetDocPos (endOffset, paraNo);		
	ScrollIfNeeded (paraNo, startOffset, endOffset);
	int viewCol = _viewPort->ColDocToWin (endOffset, _doc.GetParagraph (paraNo));
    int viewLine = _viewPort->ParaToWin (paraNo);
	_caret->Position (viewCol, viewLine);
	DocPosNotify ();
	return true;
}

void EditPaneController::Replace (ReplaceRequest const * replaceRequest)
{
	if (CanEdit () && IsSelectedForReplace (replaceRequest))
	{			
		Selection::Marker selection (_selection);
		_selection.Clear ();
		Assert (selection.IsSegment ());
		LoggingAction xLog (EditLog::Action::replace, 
							_viewPort->GetDocPosition (), 
							selection, 
							_undoLog, 
							_redoLog);
		_doc.Replace (selection.GetParaSegment (), replaceRequest->GetSubstitution ());
		Selection::ParaSegment seg = selection.GetParaSegment ();
		int paraNo = seg.ParaNo ();
		int startOffset = seg.ParaOffset ();
		int endOffset = startOffset + replaceRequest->GetSubstitution ().size ();
		_selection.CreateSelection (paraNo, startOffset, endOffset);	
		_viewPort->SetDocPos (endOffset, paraNo);
		xLog.Commit (_viewPort->GetDocPosition (), Selection::Marker (_selection));
		if (!FindNext (replaceRequest))
		{			
			ScrollIfNeeded (paraNo, startOffset, endOffset);
			int viewCol = _viewPort->ColDocToWin (endOffset, _doc.GetParagraph (paraNo));
			int viewLine = _viewPort->ParaToWin (paraNo);
			_caret->Position (viewCol, viewLine);
			DocPosNotify ();
		}
	}
	else
		FindNext (replaceRequest);
}

void EditPaneController::ReplaceAll (ReplaceRequest const * replaceRequest)
{
	if (CanEdit () && _doc.GetParagraphCount () != 0)
	{
		LoggingAction xLog (EditLog::Action::replaceAll, 
							_viewPort->GetDocPosition (), 
							Selection::Marker (_selection), 
							_undoLog, 
							_redoLog);
		_selection.Clear ();
		DocPosition curPos = _viewPort->GetDocPosition ();
		int count = _doc.ReplaceAll (replaceRequest, curPos);
		if (curPos.ParaOffset () != _viewPort->GetCurDocCol ())
		{
			_viewPort->SetDocPosition (curPos);
			int viewCol = _viewPort->ColDocToWin (curPos.ParaOffset (), _doc.GetParagraph (curPos.ParaNo ()));
			int viewLine = _viewPort->ParaToWin (curPos.ParaNo ());
			_caret->Position (viewCol, viewLine);
			DocPosNotify ();
		}
		xLog.Commit (_viewPort->GetDocPosition (), Selection::Marker (_selection));
		std::string info (ToString (count));
		info += " occurrence(s) replaced";
		TheOutput.DisplayModal (info.c_str ());	
	}
}

bool EditPaneController::EqualsSelection (Selection::ParaSegment const & segment)
{
	if ( _selection.IsEmpty ())
		return false;
	Selection::Marker selection (_selection);
	return selection.IsEqual (segment);
}

void EditPaneController::ToggleLineBreaking (bool lineBreakingOn)
{
	int oldVerScrolPos = _viewPort->GetVerScrollPos ();
	if (lineBreakingOn)
		_viewPort.reset (new BreakViewPort (_viewPort.get (), _doc));
	else
		_viewPort.reset (new ScrollViewPort (_viewPort.get (), _doc));

	_doc.SetParaNotificationSink (_viewPort.get ());// pass viewport as notification sink
	_selection.SetSelectionNotificationSink (_viewPort.get ());// pass viewport as notification sink

	int newVerScrolPos = _viewPort->GetVerScrollPos ();
	_h.Scroll (0, - _mapper->LineToY (newVerScrolPos - oldVerScrolPos));

	Win::UserMessage msg (UM_UPDATESCROLLBARS);	
	_hwndParent.SendMsg (msg);
	
	if (_h.HasFocus ())
		SetCaret ();
	_h.SetClassHRedraw (lineBreakingOn);		
	_h.ForceRepaint ();
}

void EditPaneController::SetCaret ()
{
	if (_h.HasFocus ())
	{
		int docParagraph = _viewPort->GetCurPara ();
		int docCol  = _viewPort->GetCurDocCol ();	
		int visCol = _viewPort->ColDocToWin (docCol, _doc.GetParagraph (docParagraph));
		int visLine = _viewPort->ParaToWin (docParagraph, docCol);	
		_caret->Position (visCol, visLine);
	}
}

int EditPaneController::GetScrollPosition (int bar)
{
	if (bar == SB_VERT)
		return _viewPort->GetVerScrollPos ();
	Assert(bar == SB_HORZ);
	return _viewPort->GetColOffset ();
}

void EditPaneController::ClearEditState ()
{
	_undoLog.Clear ();
	_redoLog.Clear ();
	_viewPort->GoToDocBegin ();
	Win::UserMessage msg (UM_UPDATESCROLLBARS);
	_hwndParent.SendMsg (msg);
	DocPosNotify ();
}

// LineNotyficationSink :
void EditPaneController::InvalidateLines (int startVisLine, int count)
{ 
	Win::Rect rect;
	rect.left = 0; 
	rect.top = _mapper->LineToY (startVisLine);
	rect.right = _cx; 
	rect.bottom = rect.top + count * _mapper->LineHeight (); 
	_h.Invalidate (rect);
}

void EditPaneController::ShiftLines (int startVisLine, int shift)
{
	Win::Rect scrollRect; 
	_h.GetClientRect (scrollRect); 
	scrollRect.top = _mapper->LineToY (startVisLine);
	_h.Scroll (scrollRect, 0, shift * _mapper->LineHeight ());
}

void EditPaneController::UpdateScrollBars ()
{
	Win::UserMessage msg (UM_UPDATESCROLLBARS);
	_hwndParent.SendMsg (msg);
}

void EditPaneController::GoToParagraph (int paraNo)
{
	paraNo = std::max (paraNo, 0);
	paraNo = std::min (paraNo, _doc.GetParagraphCount ());
	_viewPort->SetDocPos (0, paraNo);
	ScrollToFullView (paraNo);
	ClearSelection ();
	SetCaret ();
	DocPosNotify ();
}

void EditPaneController::CopySelectionToBuf (std::string & outBuf)
{
	Selection::Marker selection (_selection);
	// Copy all lines to the  buffer
	// taking into account possible partial
	// lines in the beginning and in the end
	// First selected line
	int firstParagraphNo = selection.StartParaNo ();
	int lastParagraphNo = selection.EndParaNo ();
	int selStartOffset = selection.StartOffset ();
	int selEndOffset = selection.EndOffset ();
	EditParagraph const * firstParagraph = _doc.GetParagraph (firstParagraphNo);
	if (firstParagraphNo != lastParagraphNo || selEndOffset == PARA_END)
	{
		outBuf.append (firstParagraph->BufAt (selStartOffset), firstParagraph->Len () - selStartOffset);
		for (int paraNo = firstParagraphNo + 1; paraNo < lastParagraphNo; paraNo++)
		{
			// Add "\r\n" at the line end
			outBuf.append ("\r\n", 2);
			EditParagraph const * para = _doc.GetParagraph (paraNo);
			outBuf.append (para->Buf (), para->Len ());
		}
		// Add "\r\n" at the last middle line end
		outBuf.append ("\r\n", 2);
		// Check if the last line has to be terminated with "\r\n"
		if (lastParagraphNo != firstParagraphNo)
		{
			EditParagraph const * lastParagraph = _doc.GetParagraph (lastParagraphNo);
			outBuf.append (lastParagraph->Buf (), selEndOffset == PARA_END ? lastParagraph->Len () : selEndOffset);
			if (selEndOffset == PARA_END)
			{
				// Add "\r\n" at the line end
				outBuf.append ("\r\n", 2);
			}
		}
	}
	else
	{
		// Only part of one line selected
		outBuf.append (firstParagraph->BufAt (selStartOffset), selEndOffset - selStartOffset);
	}

}

void EditPaneController::Undo (int numberAction)
{
	_selection.Clear ();
	// close last edit action on _undoLog 
	_undoLog.Flush ();
	
	// execute undo
	UndoSink sink (_doc, _redoLog);
	Selection::Marker selMarker;
	DocPosition docPos;
	// action from undoLog must be logged on _redoLog
	_doc.SetEditLog (&_redoLog);
	for (int k = 0; k < numberAction; ++k)
	{		
		UndoParser parser (_undoLog, sink);
		parser.Execute ();
		selMarker = parser.GetSelection ();
		docPos = parser.GetPosition ();
	}
	// end undo, then restore log for "normal" edition 
	_doc.SetEditLog (& _undoLog);
	
	// restore selection
	if (selMarker.IsValid ())
		_selection.Select (selMarker);
	// restore doc position	
	SetDocPositionShow (docPos);
}

void EditPaneController::Redo (int numberAction)
{
	_selection.Clear ();
	_undoLog.Flush ();
	// execute redo
	UndoSink sink (_doc, _undoLog);
	Selection::Marker selMarker;
	DocPosition docPos;
	// action logging on the _undoLog typically invalidated the _redoLog
	// we must make it impossible
	_redoLog.SetCanClear (false);
	for (int k = 0; k < numberAction; ++k)
	{		
		UndoParser parser (_redoLog, sink);
		parser.Execute ();
		selMarker = parser.GetSelection ();
		docPos = parser.GetPosition ();
	}
	// restore the "normal" behavior of redo log
	_redoLog.SetCanClear (true);
	_undoLog.Flush ();
	// restore selection
	if (selMarker.IsValid ())
		_selection.Select (selMarker);
	// restore doc position
	SetDocPositionShow (docPos);
}								

bool EditPaneController::MultiParaTab (bool isShift)
{
// Question : are condition for MultiParaTab edit action ?
	if (!_selection.IsEmpty ())
	{
		Selection::Marker selection (_selection);
		DocPosition selBegin = selection.Start ();
		DocPosition selEnd = selection.End ();
		if (selBegin.ParaNo () != selEnd.ParaNo () || selEnd.ParaOffset () == PARA_END)
		{
			// Answer : Condition for  MultiParaTab edit action fulfilled
			_selection.Clear ();
			DocPosition curPos = _viewPort->GetDocPosition ();
			if (isShift)
			{
				// Logging edit action 
				LoggingAction xLog (EditLog::Action::multiParaTabDel, 
									curPos, 
									selection, 
									_undoLog, 
									_redoLog);
				//execute action and calculate new selection and docPosition
				int tabSize = _fontBox->GetTabSizeChar ();
	
				// begin selection
				int beginSelShift = _doc.DeleteTabOrSpaces (selBegin.ParaNo (), tabSize);
				selBegin.DecraseParaOffset (beginSelShift);
				for (int k = selBegin.ParaNo () + 1; k < selEnd.ParaNo (); ++k)
					_doc.DeleteTabOrSpaces (k, tabSize);

				// end selection
				int endSelShift = 0;
				if (selBegin.ParaNo () != selEnd.ParaNo ())
					endSelShift = _doc.DeleteTabOrSpaces (selEnd.ParaNo (), tabSize);
				if (selEnd.ParaOffset () != PARA_END)
					selEnd.DecraseParaOffset (endSelShift);

				// docPosition
				if (curPos.ParaNo () == selBegin.ParaNo ())
					curPos.DecraseParaOffset (beginSelShift);
				else
					curPos.DecraseParaOffset (endSelShift);

				Selection::Marker newSelection (selBegin, selEnd);
				_selection.Select (newSelection);
				xLog.Commit (curPos, newSelection);
			}
			else
			{
				// First loging action
				LoggingAction xLog (EditLog::Action::multiParaTabAdd, 
									curPos, 
									selection, 
									_undoLog, 
									_redoLog);

				// Caculate the new selection and docPosition,
				const EditParagraph * para = _doc.GetParagraph (selBegin.ParaNo ());
				for (int k = 0; k < selBegin.ParaOffset (); ++k)
				{
					if ((*para)[k] != '\t')
					{
						selBegin.IncraseParaOffset ();
						if (selBegin.ParaNo () == curPos.ParaNo ())
							curPos.IncraseParaOffset ();
						break;
					}
				}				
				if (selEnd.ParaOffset () != PARA_END)
				{
					selEnd.IncraseParaOffset ();
					if (selEnd.ParaNo () == curPos.ParaNo ())
						curPos.IncraseParaOffset ();
				}

				//execute the edit action
				_doc.AddTabs (selBegin.ParaNo (), selEnd.ParaNo ());

				// Set the new selection
				Selection::Marker newSelection (selBegin, selEnd);
				_selection.Select (newSelection);
				xLog.Commit (curPos, newSelection);							
			}
			// Set the new docPosition
			SetDocPositionShow (curPos); 
			DocPosNotify ();
			return true; // MultiParaTab edit action has been execute
		}
	}
	return false; // not MultiParaTab edit action
}

void EditPaneController::ChangeReadOnlyState (bool readOnly)
{
	if (readOnly)
	{
		_undoLog.Clear ();
		_redoLog.Clear ();
	}
	_isReadOnly = readOnly;
}
