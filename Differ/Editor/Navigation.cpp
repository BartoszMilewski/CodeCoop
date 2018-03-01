//
// (c) Reliable Software, 1997-2002
//

#include "precompiled.h"
#include "EditPane.h"
#include "EditParams.h"

//
// Mouse selection
//

void EditPaneController::SetPositionAdjust (int & visCol, int visLine, int & docCol, int & docPara)
{
	PositionAdjust (visCol, visLine, docCol, docPara);
	_viewPort->SetDocPos (docCol, docPara);
	ScrollToDocPos ();
}

void EditPaneController::PositionAdjust (int & visCol, int visLine, int & docCol, int & docPara)
{
    docPara = _viewPort->LineWinToPara (visLine);
	if (docPara > _doc.GetDocEndParaPos ())
		docPara = _doc.GetDocEndParaPos ();
	if (docPara < 0 )
		docPara = 0;
	EditParagraph const * para = _doc.GetParagraph (docPara);
    docCol = _viewPort->ColWinToDocAdjust (visCol, visLine, para);
	// Revisit: Hit this assertion with
	// para != 0, visCol 0d, visLine 0c, docCol 0a, docPara 0c
	// para starting with tab
	Assert (para != 0 ? visCol == _viewPort->ColDocToWin (docCol, para) : 0 == _viewPort->ColDocToWin (docCol, para));
}

void EditPaneController::SetDocPositionShow (DocPosition  docPos)
{
	int docPara = docPos.ParaNo ();
	int docCol = docPos.ParaOffset ();
	int docParagraphCount = _doc.GetParagraphCount ();
	if (docParagraphCount > 0 && docPara >= docParagraphCount)
	{
		docPara = _doc.GetParagraphCount () - 1;
		docCol = _doc.GetParagraphAlways (docPara)->Len ();
	}
	_viewPort->SetDocPos (docCol, docPara);
	_viewPort->SetAnchorCol (docCol, _doc.GetParagraph (docPara));
	ScrollToDocPos ();
}

bool EditPaneController::OnLButtonDown (int x, int y, Win::KeyState kState) throw ()
{
	// Revisit: API	
	::SetCapture (_h.ToNative ());
	int visCol = _mapper->XtoCol (x);
	int visLine = _mapper->YtoLineTrunc (y);

	if (kState.IsCtrl ())
	{
		ClearSelection ();
		SelectWord (visCol, visLine);
	}
	else if (kState.IsShift ())
	{
		int oldParaOffset = _viewPort->GetCurDocCol ();
		int oldPara = _viewPort->GetCurPara ();
		int newPara , newParaOffset;
		SetPositionAdjust (visCol, visLine, newParaOffset, newPara);
		_viewPort->SetAnchorCol (newParaOffset, _doc.GetParagraph (newPara));

		if (_selection.IsEmpty ()) 
			_selection.StartSelect (oldPara, oldParaOffset, 0);
		else if (_selection.IsSelected (newPara, newParaOffset))
			_selection.StartSelect (newPara, newParaOffset, 0);
		 
		_selection.ContinueSelect (newPara, newParaOffset);
	}
	else
	{
		int paraOffset, para;
		SetPositionAdjust (visCol, visLine, paraOffset, para);
		_viewPort->SetAnchorCol (paraOffset, _doc.GetParagraph (para));
		ClearSelection ();
		_selection.StartSelect (para, paraOffset, kState);
	}
	DocPosNotify ();
	return true;
}

bool EditPaneController::OnLButtonUp (int x, int y, Win::KeyState kState) throw ()
{
	_mousePoint.x = x;
	_mousePoint.y = y;
	// Calling ReleaseCapture will send us the WM_CAPTURECHANGED
	_h.ReleaseMouse ();
	return true;
}

void EditPaneController::LButtonDrag (int x, int y)
{
	if (!_h.HasCapture ())
		return;
	_mousePoint.x = x;
	_mousePoint.y = y;
	int visCol = _mapper->XtoCol (x);
	int visLine = _mapper->YtoLineTrunc (y);
	
	int docCol, docPara;
	SetPositionAdjust (visCol, visLine, docCol, docPara);
	_caret->Position (visCol, visLine);

	_selection.ContinueSelect (docPara, docCol);
	if (_viewPort->LineIsOutside (visLine) || _viewPort->ColIsOutside (visCol))
	{
		_timer.Set (100);
	}
}

bool EditPaneController::OnLButtonDblClick (int x, int y, Win::KeyState kState) throw ()
{
	int visCol = _mapper->XtoCol (x);
	int visLine = _mapper->YtoLineTrunc (y);
	SelectWord (visCol, visLine);
	return true;
}

void EditPaneController::SelectWord (int visCol, int visLine)
{
	int docCol, docPara;
	SetPositionAdjust (visCol, visLine, docCol, docPara);
	EditParagraph const * para = _doc.GetParagraph (docPara);
	if (para == 0)
		return;	// No document -- cannot select word
	int wordBegin, wordEnd;
	_viewPort->SelectWord (wordBegin, wordEnd, para);
	_viewPort->SetAnchorCol (wordBegin, para);
	visCol = _viewPort->ColDocToWin (wordEnd, para);
	_caret->Position (visCol, visLine);
	SetPositionAdjust (visCol, visLine, docCol, docPara);

	if (wordBegin == 0 && wordEnd > para->Len ())
		wordEnd = PARA_END;
	_selection.CreateSelection (docPara, wordBegin, wordEnd);
}

bool EditPaneController::OnCaptureChanged (Win::Dow::Handle newCaptureWin) throw ()
{
	// End drag selection -- for what ever reason
	int visCol = _mapper->XtoCol (_mousePoint.x);
	int visLine = _mapper->YtoLineTrunc (_mousePoint.y);
	int docCol, docPara;
	PositionAdjust (visCol, visLine, docCol, docPara);
	_selection.EndSelect (docPara, docCol);
	if (docCol == PARA_END)
	{
		docPara++;
		docCol = 0;
	}

	SetDocPositionShow (DocPosition (docPara,docCol));
	_viewPort->SetAnchorCol (docCol, _doc.GetParagraph (docPara));
	return true;
}

void EditPaneController::StartSelectLine (int flags, int y)
{
	int visLine = _mapper->YtoLineTrunc (y);
	int paraOffsetBegin;
	int paraOffsetEnd;
	_viewPort->GetLine (visLine, paraOffsetBegin, paraOffsetEnd);
	int visCol = 0;

	int docCol, docPara;
	SetPositionAdjust (visCol, visLine, docCol, docPara);
	if (docPara <  _doc.GetParagraphCount ())
	{
		_viewPort->SetAnchorCol (docCol, _doc.GetParagraph (docPara));
		_selection.StartSelectLine (docPara, paraOffsetBegin, paraOffsetEnd, flags);
	}
}

void EditPaneController::ContinueSelectLine (int y)
{
	int visLine = _mapper->YtoLineTrunc (y);
	int visCol = 0;
	int paraOffsetBegin;
	int paraOffsetEnd;
	_viewPort->GetLine (visLine, paraOffsetBegin, paraOffsetEnd);
	int docCol, docPara;
	SetPositionAdjust (visCol, visLine, docCol, docPara);
	if (docPara <  _doc.GetParagraphCount ())			
		_selection.ContinueSelectLine (docPara, paraOffsetBegin, paraOffsetEnd);
}

void EditPaneController::EndSelectLine (int y)
{
	int visCol = 0;
	int visLine = _mapper->YtoLineTrunc (y);
	int paraOffsetBegin;
	int paraOffsetEnd;
	_viewPort->GetLine (visLine, paraOffsetBegin, paraOffsetEnd);
	int docCol, docPara;
	PositionAdjust (visCol, visLine, docCol, docPara);

	if (docPara <  _doc.GetParagraphCount ())
	{
		bool paraSelection  = _selection.IsParaSelection ();
		bool isExtDown = _selection.EndSelectLine (docPara, paraOffsetBegin, paraOffsetEnd);
		// reposition the caret
		if (isExtDown)
		{
			if (paraSelection)
			{
				visLine = _viewPort->ParaToWin (docPara, _doc.GetParagraph (docPara)->Len ());
			}
			
			if (docPara > 0 && docPara == _doc.GetDocEndParaPos ())
			{
				// it's the last line
				visCol = _doc.GetParagraphLen (docPara);
			}
			else
			{
				visLine++;
			}
		}
		else if (paraSelection)
		{
			visCol = 0;
			visLine = _viewPort->ParaToWin (docPara, 0);
		}
	}
	SetPositionAdjust (visCol, visLine, docCol, docPara);
}

//
// Selection helpers
//

void EditPaneController::ClearSelection ()
{
	_selection.Clear ();
	DocPosNotify ();
}

// Called for keyboard navigation with shift
void EditPaneController::AdjustSelection (int docColPos, int docParaPos, int prevDocColPos, int prevDocParaPos)
{
	_selection.ContinueSelect (docParaPos, docColPos, prevDocParaPos, prevDocColPos);
}

void EditPaneController::RefreshLineSegment (int docPara, int offsetBegin, int offsetEnd)
{
	EditParagraph const * para = _doc.GetParagraph (docPara);
	if (offsetBegin == PARA_END)
		offsetBegin = para->Len ();
	if (offsetEnd == PARA_END)
		offsetEnd = para->Len ();

	int visColA = _viewPort->ColDocToWin (offsetBegin, para);
	int visColB = _viewPort->ColDocToWin (offsetEnd, para);
	Win::Rect rect;
	rect.left = _mapper->ColToX (std::min (visColA, visColB));
	rect.right = _mapper->ColToX (std::max (visColA, visColB));
	int visLine = _viewPort->ParaToWin (docPara, offsetBegin);
	rect.top = _mapper->LineToY (visLine);
	rect.bottom = rect.top + _mapper->LineHeight ();
	// Revisit: API
	::InvalidateRect (_h.ToNative (), &rect, TRUE);
}

void EditPaneController::ScrollToDocPos () 
{
	int paraOfset = _viewPort->GetCurDocCol ();
	int docPara = _viewPort->GetCurPara ();
	int viewCol = _viewPort->ColDocToWin (paraOfset, _doc.GetParagraph (docPara));
    int viewLine = _viewPort->ParaToWin (docPara, paraOfset);
	int newPos;
	if (viewLine < 0)
	{
		newPos = _viewPort->ParaToLine (docPara, paraOfset);
		int lastPara = _doc.GetParagraphCount () - 1;
		Assert (lastPara >=0);
		int lastPos = _viewPort->ParaToLine (lastPara, _doc.GetParagraph (lastPara)->Len ());
		newPos = lastPos - newPos < _viewPort->Lines () ? lastPos - _viewPort->Lines () + 1 : newPos;

		Win::UserMessage setVScroll (UM_SETSCROLLBAR, SB_VERT, newPos);
		_hwndParent.SendMsg (setVScroll);
	}
	else if (viewLine >= _viewPort->Lines () - 1 && viewLine > 0)
	{
		newPos = _viewPort->ParaToLine (docPara, paraOfset);
		if (_viewPort->Lines () >= 2)
			newPos += -_viewPort->Lines () + 2; 	
		Win::UserMessage setVScroll (UM_SETSCROLLBAR, SB_VERT, newPos);
		_hwndParent.SendMsg (setVScroll);
	}

	if (viewCol < 0)
	{
		newPos = viewCol + _viewPort->GetColOffset ();
		Win::UserMessage setHScroll (UM_SETSCROLLBAR, SB_HORZ, newPos);
		_hwndParent.SendMsg (setHScroll);
	}
	else if (!_viewPort->AdjustDocPos (_doc.GetParagraph (docPara)))
	{
		newPos = viewCol + _viewPort->GetColOffset () - _viewPort->GetVisCols () + 2;
		Win::UserMessage setHScroll (UM_SETSCROLLBAR, SB_HORZ, newPos);
		_hwndParent.SendMsg (setHScroll);
	}
	SetCaret ();
}

void EditPaneController::SynchScrollToDocPos (int offset, int targetPara)
{
	// Find paragraph whose "remembered" para number is the same
	for (int i = 0; i < _doc.GetParagraphCount ();  i++)
	{		
		EditParagraph const * para = _doc.GetParagraph (i);
		// if paragraph was "moved," skip the original "removed" position
		if (para->GetParagraphNo () == targetPara && !para->GetStyle ().IsRemoved ())
		{
			int line = _viewPort->ParaToLine (i, offset);
			SetDocPosition (SB_VERT, line, false);
			if (offset > para->Len ())
			{
				offset = para->Len ();
			}
			_viewPort->SetDocPos (offset, i);
			return;
		}
	}	
}

void EditPaneController::ScrollToFullView (int docLine)
{
	int docLines =_viewPort->GetDocLines (_doc);
	int docLinesLeft =  docLines - docLine;
	int topVisibleLine = 0;
	if (docLinesLeft >= _viewPort->Lines ())
	{
		topVisibleLine = docLine > 2 ? docLine - 2 : 0;
	}
	else
	{
		// end of document in view
		if (_viewPort->Lines () < docLines) // whole document doesn't fit
			topVisibleLine = docLines - _viewPort->Lines ();
	}
	Win::UserMessage setVScroll (UM_SETSCROLLBAR, SB_VERT, topVisibleLine);
	_hwndParent.SendMsg (setVScroll);	
}

void EditPaneController::ScrollIfNeeded (int docPara, int offsetBegin, int offsetEnd)
{	
	if (_viewPort->ParaToWinClip (docPara, offsetBegin) == -1
	 || _viewPort->ParaToWinClip (docPara, offsetEnd) == -1)
	{
		// paragraph not in view
		int line = _viewPort->ParaToLine (docPara, offsetBegin);
		ScrollToFullView (line);
	}
	int colBegin = _viewPort->ColDocToWin (offsetBegin, _doc.GetParagraph (docPara));
	int colEnd = _viewPort->ColDocToWin (offsetEnd, _doc.GetParagraph (docPara));	
	if (_viewPort->ColIsOutside (colBegin - 1)
	 || _viewPort->ColIsOutside (colEnd + 1))
	{
		// selection is clipped horizontally
		int leftVisCol = (colEnd + colBegin - _viewPort->GetVisCols ())/2 + _viewPort->GetColOffset ();
		leftVisCol = std::max (leftVisCol, 0);
		Win::UserMessage setHScroll (UM_SETSCROLLBAR, SB_HORZ, leftVisCol);
	   _hwndParent.SendMsg (setHScroll);	   	  
	}
}

//
// Navigation
//

void EditPaneController::NextChange ()
{
	ClearSelection ();
	int docCol = 0;
    int docPara = _doc.NextChange (_viewPort->GetCurPara ());
	bool noMoreChanges = (docPara == -1);
    if (noMoreChanges)
    {
        // Stay at the document end
        docPara = _doc.GetDocEndParaPos ();
    }
    _viewPort->SetDocPos (docCol, docPara);
	ScrollToDocPos ();
	if (!noMoreChanges)
	{
		int line = _viewPort->ParaToLine (docPara, 0);
		ScrollToFullView (line);
	}	
	DocPosNotify ();
}

void EditPaneController::PrevChange ()
{
	ClearSelection ();
	int docCol = 0;
    int docPara = _doc.PrevChange (_viewPort->GetCurPara ());
	bool noMoreChanges = (docPara == -1);
    if (noMoreChanges)
    {
        // Stay at the document begin
        docPara = 0;
    }
    _viewPort->SetDocPos (docCol, docPara);
	ScrollToDocPos ();
	if (!noMoreChanges)
	{
		int line = _viewPort->ParaToLine (docPara, 0);
		ScrollToFullView (line);
	}
	DocPosNotify ();
}

void EditPaneController::Up (bool isShift, bool isCtrl)
{
    if (isCtrl)
    {
		ClearSelection ();
		_hwndParent.SendMsg (WM_VSCROLL, MAKEWPARAM (SB_LINEUP, 0));
    }
    else
    {
		ModifySelection (isShift);
		int prevDocCol = _viewPort->GetCurDocCol ();
        int prevDocPara = _viewPort->GetCurPara ();
	    int line = _viewPort->Up ();
		int docPara = _viewPort->LineToPara (line);
		int docCol = _viewPort->GetDocAnchorCol (_doc.GetParagraph (docPara), line);
		int visCol = _viewPort->GetVisibleAnchorCol ();
        int visLine = _viewPort->ParaToWin (docPara, docCol);
        SetPositionAdjust (visCol, visLine, docCol, docPara);
        if (isShift)
            AdjustSelection (docCol, docPara, prevDocCol, prevDocPara);
    }
	DocPosNotify ();
}

void EditPaneController::Down (bool isShift, bool isCtrl)
{
    if (isCtrl)
    {
		ClearSelection ();
		_hwndParent.SendMsg (WM_VSCROLL, MAKEWPARAM (SB_LINEDOWN, 0));
    }
    else
    {
		ModifySelection (isShift);
		int prevDocCol = _viewPort->GetCurDocCol ();
        int prevDocPara = _viewPort->GetCurPara ();
		int line = _viewPort->Down (_doc.GetDocEndParaPos ());
	    int docPara = _viewPort->LineToPara (line);
		int docCol = _viewPort->GetDocAnchorCol (_doc.GetParagraph (docPara), line);
		int visCol = _viewPort->GetVisibleAnchorCol ();
        int visLine = _viewPort->ParaToWin (docPara, docCol);
        SetPositionAdjust (visCol, visLine, docCol, docPara);
        if (isShift)
            AdjustSelection (docCol, docPara, prevDocCol, prevDocPara);
    }
	DocPosNotify ();
}

void EditPaneController::Left (bool isShift, bool isCtrl)
{
	int docPara = _viewPort->GetCurPara ();
	int prevDocCol = _viewPort->GetCurDocCol ();
	int prevDocPara = docPara;
	int docCol = prevDocCol;
	if (!_selection.IsEmpty () && !isShift && !isCtrl)
	{
		// behaviour : move the cursor for a selection's start 
		Selection::Marker selection (_selection);
		_selection.Clear ();
		docPara = selection.StartParaNo ();
		docCol = selection.StartOffset ();		
	}
	else
	{
		ModifySelection (isShift);		
		int prevParaLen = docPara > 0 ? _doc.GetParagraphLen (docPara - 1) : 0;
		if (isCtrl)
			docCol = _viewPort->LeftWord (docPara, _doc.GetParagraph (docPara), prevParaLen);
		else
		{		 
			EditParagraph const * currPara = _doc.GetParagraph (docPara);
			EditParagraph const * prevPara = 0;
			if (docPara > 0)
				prevPara = _doc.GetParagraph (docPara - 1);
			docCol = _viewPort->Left (docPara, currPara, prevPara);
		}
	}
	_viewPort->SetAnchorCol (docCol, _doc.GetParagraph (docPara));
    int visCol = _viewPort->ColDocToWin (docCol, _doc.GetParagraph (docPara));
    int visLine = _viewPort->ParaToWin (docPara, docCol);
    SetPositionAdjust (visCol, visLine, docCol, docPara);
    if (isShift)
        AdjustSelection (docCol, docPara, prevDocCol, prevDocPara);
	DocPosNotify ();
}

void EditPaneController::Right (bool isShift, bool isCtrl)
{
	int docPara = _viewPort->GetCurPara ();
	int prevDocCol = _viewPort->GetCurDocCol ();
	int prevDocPara = docPara;
	int docCol = prevDocCol;
	if (!_selection.IsEmpty () && !isShift && !isCtrl)
	{
		// behaviour : move the cursor for a selection's end 
		Selection::Marker selection (_selection);
		_selection.Clear ();
		docPara = selection.EndParaNo ();
		docCol = selection.EndOffset ();
		if (docCol == PARA_END)
		{
			EditParagraph const * para = _doc.GetParagraph (docPara);
			Assert (para != 0);
			docCol = para->Len ();
		}		
	}
	else
	{
		ModifySelection (isShift);		
		if (isCtrl)
			docCol = _viewPort->RightWord (docPara, _doc.GetParagraph (docPara), _doc.GetDocEndParaPos ());
		else
		{
			EditParagraph const * currPara = _doc.GetParagraph (docPara);
			docCol = _viewPort->Right (docPara, currPara, _doc.GetDocEndParaPos ());
		}
	}
	_viewPort->SetAnchorCol (docCol, _doc.GetParagraph (docPara));
    int visCol = _viewPort->ColDocToWin (docCol, _doc.GetParagraph (docPara));
    int visLine = _viewPort->ParaToWin (docPara, docCol);
    SetPositionAdjust (visCol, visLine, docCol, docPara);
    if (isShift)
        AdjustSelection (docCol, docPara, prevDocCol, prevDocPara);
	DocPosNotify ();
}

void EditPaneController::PageDown (bool isShift)
{
	ModifySelection (isShift);
	int prevDocCol = _viewPort->GetCurDocCol ();
    int prevDocPara = _viewPort->GetCurPara ();
    int line = _viewPort->PageDown (_doc.GetDocEndParaPos ());
	int docPara = _viewPort->GetCurPara ();	
	int docCol = _viewPort->GetDocAnchorCol (_doc.GetParagraph (docPara), line);
	int visCol = _viewPort->GetVisibleAnchorCol ();
    int visLine = _viewPort->ParaToWin (docPara, docCol);
    SetPositionAdjust (visCol, visLine, docCol, docPara);
    if (isShift)
        AdjustSelection (docCol, docPara, prevDocCol, prevDocPara);
	DocPosNotify ();
}

void EditPaneController::PageUp (bool isShift)
{
	ModifySelection (isShift);
	int prevDocCol = _viewPort->GetCurDocCol ();
    int prevDocPara = _viewPort->GetCurPara ();
    int line = _viewPort->PageUp ();
	int docPara = _viewPort->GetCurPara ();
	int docCol = _viewPort->GetDocAnchorCol (_doc.GetParagraph (docPara), line);
	int visCol = _viewPort->GetVisibleAnchorCol ();
    int visLine = _viewPort->ParaToWin (docPara);
    SetPositionAdjust (visCol, visLine, docCol, docPara);
    if (isShift)
        AdjustSelection (docCol, docPara, prevDocCol, prevDocPara);
	DocPosNotify ();
}

void EditPaneController::Home (bool isShift, bool isCtrl)
{
	ModifySelection (isShift);
	int prevDocCol = _viewPort->GetCurDocCol ();
    int prevDocPara = _viewPort->GetCurPara ();
    if (isCtrl)
        _viewPort->DocBegin ();
    else
        _viewPort->LineBegin ();
	int docPara = _viewPort->GetCurPara ();
    int docCol  = _viewPort->GetCurDocCol (); 
    int visCol = _viewPort->ColDocToWin (docCol, _doc.GetParagraph (docPara));
    int visLine = _viewPort->ParaToWin (docPara, docCol);
    SetPositionAdjust (visCol, visLine, docCol, docPara);
	if (isShift)
		AdjustSelection (docCol, docPara, prevDocCol, prevDocPara);
	DocPosNotify ();
}

void EditPaneController::End (bool isShift, bool isCtrl)
{
	ModifySelection (isShift);
	int prevDocCol = _viewPort->GetCurDocCol ();
    int prevDocPara = _viewPort->GetCurPara ();
    if (isCtrl)
        _viewPort->DocEnd (_doc.GetParagraphLen (_doc.GetDocEndParaPos ()), _doc.GetDocEndParaPos ());
    else
        _viewPort->LineEnd (_doc.GetParagraphLen (_viewPort->GetCurPara ()));
	int docPara = _viewPort->GetCurPara ();
    int docCol  = _viewPort->GetCurDocCol ();
	_viewPort->SetAnchorCol (docCol, _doc.GetParagraph (docPara));
    int visCol = _viewPort->ColDocToWin (docCol, _doc.GetParagraph (docPara));
    int visLine = _viewPort->ParaToWin (docPara, docCol);
    SetPositionAdjust (visCol, visLine, docCol, docPara);
	if (isShift)
		AdjustSelection (docCol, docPara, prevDocCol, prevDocPara);
	DocPosNotify ();
}

// Called after keyboard navigation command (arrow, pgUp, etc...)

void EditPaneController::ModifySelection (bool isShift)
{
	if (isShift)
	{
		if (_selection.IsEmpty ())
			_selection.StartSelect (_viewPort->GetCurPara (), _viewPort->GetCurDocCol (), 0);
	}
	else
	{
		ClearSelection ();
		_selection.StartSelect (_viewPort->GetCurPara (), _viewPort->GetCurDocCol (), 0);
	}
}

void EditPaneController::DocPosNotify () const
{
	int curParaNo = _viewPort->GetCurPara ();
	Win::UserMessage vertPos (UM_DOCPOS_UPDATE, SB_VERT, curParaNo + 1);
	_hwndParent.SendMsg (vertPos);
	int curDocCol = _viewPort->GetCurDocCol ();
	EditParagraph const * para = _doc.GetParagraph (curParaNo);
	curDocCol = _viewPort->GetExpandedDocCol (curDocCol, para);
	Win::UserMessage horzPos (UM_DOCPOS_UPDATE, SB_HORZ, curDocCol + 1);
	_hwndParent.SendMsg (horzPos);
}
