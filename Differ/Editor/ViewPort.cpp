//
// (c) Reliable Software 1997-2003
//
#include "precompiled.h"
#include "ViewPort.h"
#include "Lines.h"
#include "Document.h"
#include "EditPane.h"
#include "Select.h"
#include <StringOp.h>

ViewPort::ViewPort (LineNotificationSink * sink, int tabOff) 
	: _tabber (tabOff),
      _visCols (0),
      _visLines (0),
      _docCol (0),
      _docPara (0),
      _anchorCol (0),
	  _maxLineLen (0),
	  _lineSink (sink)
{}

// Lines
bool ViewPort::LineIsOutside (int line) const
{
	return (line < 0 || line >= _visLines);
}

// Columns
int ViewPort::ColDocToWinClip (int docCol, EditParagraph const * para) const 
{
	int visCol = ColDocToWin (docCol, para); 
	if (visCol > _visCols)
		visCol = _visCols;
	return visCol;
}

void ViewPort::SetAnchorCol (int docCol, EditParagraph const * curPara)
{
	// Anchor column is the absolute window column
	_anchorCol = ColDocToWin (docCol, curPara) + _scrollPos._horScrollPos;
}

int ViewPort::GetVisibleAnchorCol () const
{
	return _anchorCol - _scrollPos._horScrollPos;
}

//
// Navigation
//

void ViewPort::SetDocPos (int docCol, int docPara)
{
    _docCol = docCol;
    _docPara = docPara;
}

void ViewPort::SetDocPosition  (DocPosition const & docPos)
{
	_docCol = docPos.ParaOffset ();
	_docPara = docPos.ParaNo ();
}


int ViewPort::LeftWord (int & docPara, EditParagraph const * curPara, int prevLineLen)
{
    if (curPara == 0)
        return _docCol;

    int wordBegin = 0;
    if (_docCol == 0)
    {
        _docPara--;
        if (_docPara == -1)
            _docPara = 0;
        docPara = _docPara;
        wordBegin = prevLineLen;
    }
    else
    {
        wordBegin = PrevWord (curPara->Buf ());
        if (wordBegin == -1)
        {
            wordBegin = 0;
        }
    }
    _docCol = wordBegin;
    return wordBegin;
}

int ViewPort::RightWord (int & docPara, EditParagraph const * curPara, int docLen)
{
    if (curPara == 0)
        return _docCol;

    int wordBegin = 0;
    if (_docCol == curPara->Len ())
    {
        _docPara++;
        if (_docPara > docLen)
            _docPara = docLen;
        docPara = _docPara;
    }
    else
    {
        wordBegin = NextWord (curPara->Buf (), curPara->Len ());
        if (wordBegin == -1)
        {
            wordBegin = curPara->Len ();
        }
    }
    _docCol = wordBegin;
    return wordBegin;
}

void ViewPort::SelectWord (int & wordBegin, int & wordEnd, EditParagraph const * para)
{
	if (para == 0)
	{
		wordBegin = _docCol;
		wordEnd   = _docCol;
		return;
	}

	int lineLen = para->Len ();
	char const * lineText = para->Buf ();
	int i = _docCol;
	if (::IsSpace (lineText [i]) && i - 1 > 0 && !::IsWordBreak (lineText [i-1]))
	{
		// Word is to the left
		i--;
	}
	// Skip C language symbol -- go to the word begin
	while (i > 0 && !::IsWordBreak (lineText [i - 1]))
	{
		i--;
	}
	wordBegin = i;
    // Skip C language symbol -- go to the word end
    while (i < lineLen && !::IsWordBreak (lineText [i]))
    {
        i++;
    }
	if (i == wordBegin && i + 1 <= lineLen && ::IsGraph (lineText [i]))
	{
		// We didn't move -- select single char in front of the cursor
		// if this is printable character
		i++;
	}
	wordEnd = i;
	Assert (wordBegin <= wordEnd);
}

void ViewPort::DocBegin ()
{
    _docCol = 0;
    _docPara = 0;
    _anchorCol = 0;
}

void ViewPort::LineBegin ()
{
    _docCol = 0;
    _anchorCol = 0;
}

void ViewPort::DocEnd (int lastLineLen, int docLen)
{
    _docCol = lastLineLen;
    _docPara = docLen;
}

void ViewPort::LineEnd (int lineLen)
{
    _docCol = lineLen;
}

int ViewPort::PrevWord (char const * line)
{
    Assert (line != 0);
    int i = _docCol - 1;
    if (i == -1)
        return i;

    // Skip non-alphabetic chars
    while (::IsWordBreak (line [i]))
    {
        i--;
        if (i == -1)
            return -1;
    }
    // Skip alphabetic chars -- go to the previous word begin
    while (!::IsWordBreak (line [i]))
    {
        i--;
        if (i == -1)
            return -1;
    }
    return i + 1;
}

int ViewPort::NextWord (char const * line, int lineLen)
{
    Assert (line != 0);
    int i = _docCol;
    // Skip alphabetic chars -- the current word
    while (!::IsWordBreak (line [i]))
    {
        i++;
        if (i > lineLen)
            return -1;
    }
    // Skip non-alphabetic chars -- go to the next word begin
    while (::IsWordBreak (line [i]))
    {
        i++;
        if (i > lineLen)
            return -1;
    }
    return i;
}

void ViewPort::AdjustDocPos (const Document & doc)
{
	int paraCount = doc.GetParagraphCount ();
	if (paraCount == 0)
	{
		_docPara = 0;
	    _docCol = 0;
	}
	else
	{
		_docPara = std::min (_docPara, paraCount - 1);
		_docCol = std::min (_docCol, doc.GetParagraph (_docPara)->Len ());
	}
}

int ViewPort::GetExpandedDocCol (int col, EditParagraph const * para) const
{
	if (para == 0)
		return 0;
	Assert (col <= para->Len ());
	int expandCol = _tabber.ExpandTabs (para, 0, col, 0);
	return expandCol;
}

//--------------------------------------------------------------

//ScrollViewPort
ScrollViewPort::ScrollViewPort (const ViewPort * otherPort, const Document & doc)
	: ViewPort (*otherPort)
{
	int count = doc.GetParagraphCount (); 
	for (int i = 0; i < count; i++) 
	{ 
		UpdateMaxParagraphLen (doc.GetParagraph (i)); 
	}
	_scrollPos._docParaOffset = 0;
}
//lines
int ScrollViewPort::LineWinToPara (int visLine, int & offset) const 
{
	offset = 0;
	int docLine = visLine + _scrollPos._docPara;
	if (docLine < 0)
		return 0;
	return docLine;
}

bool ScrollViewPort::UpdateMaxParagraphLen (EditParagraph const * para)
{
	int tabExpandedLineLen = _tabber.ExpandTabs (para, 0, para->Len (), 0);
	if (tabExpandedLineLen > _maxLineLen)
	{
		_maxLineLen = tabExpandedLineLen;
		return true;
	}
	return false;
}

int ScrollViewPort::SetScrollPosition (EditParagraph const * para, int line)
{
	_scrollPos._docParaOffset = VisibleParaOffset (para);	
    int delta = line - _scrollPos._docPara;
    _scrollPos._docPara = line;
    return delta;
}

//columns
int ScrollViewPort::SetDocCol (int docCol)
{
	int delta = docCol - _scrollPos._horScrollPos;
	_scrollPos._horScrollPos = docCol;
	return delta;
}

bool ScrollViewPort::ColIsOutside (int visCol) const
{
	return (visCol < 0 || visCol >= _visCols);
}

int ScrollViewPort::ColDocToWin (int docCol, EditParagraph const * para) const 
{
	if (para == 0)
		return docCol - _scrollPos._horScrollPos;

	// # of columns after end of line
	int tail = docCol - para->Len ();
	int len = docCol;
	if (tail > 0) // we are past the line end
		len = para->Len ();

	// doc column with tabs expanded
	int expandCol = _tabber.ExpandTabs (para, 0, len, 0);

	if (tail > 0) // past the line end
		expandCol += tail;

	return expandCol - _scrollPos._horScrollPos; 
}

// If visCol falls in the middle of a tab, stop at the tab.
// If it falls beyond the end of line, stop at the end of line.
// Adjust visCol to reflect the actual stop position
int ScrollViewPort::ColWinToDocAdjust (int & visCol, int & visLine, EditParagraph const * para)  const 
{
	if (para == 0)
		return 0;
	
	// Find last such character that printing
	// the next one would move insertion point
	// beyond the target column
	int len = para->Len ();
	// Start expanding from the beginning of line
	int expTargetCol = _scrollPos._horScrollPos + visCol;
	int expandCol = 0;
	int i;
	for (i = 0; i < len; i++)
	{
		int nextCol = expandCol + 1;
		if ((*para) [i] == '\t')
			nextCol = _tabber.SkipTab (nextCol);
		if (nextCol > expTargetCol)
			break;
		expandCol = nextCol;
	}
	// Adjust the visible column
	visCol = expandCol - _scrollPos._horScrollPos;
	return i;
}

int ScrollViewPort::VisibleParaOffset (EditParagraph const * para)  const 
{
	int visCol = 0;
	int visLine;
	return ColWinToDocAdjust (visCol, visLine, para);
}

int ScrollViewPort::GetDocAnchorCol (EditParagraph const * curPara, int line) const
{
	int visLine;
	int anchorTmp = _anchorCol;
	return ColWinToDocAdjust (anchorTmp, visLine, curPara);
}

// navigation

int ScrollViewPort::Up ()
{
    _docPara--;
    if (_docPara == -1)
        _docPara = 0;
    return _docPara;
}

int ScrollViewPort::Down (int docLen)
{
    _docPara++;
    if (_docPara > docLen)
        _docPara = docLen;
    return _docPara;
}

int ScrollViewPort::PageDown (int docLen)
{
    _docPara += (_visLines - 1);
    if (_docPara > docLen)
        _docPara = docLen;
	return _docPara;
}

int ScrollViewPort::PageUp ()
{
    _docPara -= (_visLines - 1);
    if (_docPara < 0)
        _docPara = 0;
	return _docPara;
}

int ScrollViewPort::Left (int & currParaNo, EditParagraph const * currPara, EditParagraph const * prevPara)
{
    _docCol--;
    if (_docCol == -1)
    {
		if (prevPara != 0)
           _docCol = prevPara->Len ();
		else
			_docCol = 0;
        _docPara--;
        if (_docPara == -1)
            _docPara = 0;
        currParaNo = _docPara;
    }
    return _docCol;
}

int ScrollViewPort::Right (int & docPara, EditParagraph const * currPara, int docLen)
{
	if (currPara == 0)
		return 0;

    _docCol++;
    if (_docCol > currPara->Len ())
    { 
		_docCol = 0;
        _docPara++;
        if (_docPara > docLen)
		{
            _docPara = docLen;
			_docCol = currPara->Len ();
		}		
        docPara = _docPara;
    }
    return _docCol;
}

void ScrollViewPort::NewDoc (const Document & doc)
{
	ViewPort::AdjustDocPos (doc);
}

void ViewPort::GoToDocBegin ()
{
	_docCol = 0;
	_docPara = 0;
	_scrollPos._docPara = 0;
	_scrollPos._docParaOffset = 0;
	_scrollPos._horScrollPos = 0;
}

int ScrollViewPort::GetDocLines (const Document & doc) const
{
	return doc.GetParagraphCount ();
}

void ScrollViewPort::OnParaLenChange (DocPosition const & pos, EditParagraph const * para, int deltaLen)
{
	if(	UpdateMaxParagraphLen (para))
		_lineSink->UpdateScrollBars ();
	int line = pos.ParaNo () - _scrollPos._docPara;
	_lineSink->InvalidateLines (line, 1);
}

void ScrollViewPort::OnStyleChanged (int paraNo)
{
	int line = paraNo - _scrollPos._docPara;
	_lineSink->InvalidateLines (line, 1);
}

void ScrollViewPort::OnMergeParagraphs (int startDocPara, EditParagraph const * newPara)
{
	UpdateMaxParagraphLen (newPara);	
	int line = startDocPara -_scrollPos._docPara;
	_lineSink->InvalidateLines (line, 2);
	_lineSink->ShiftLines (line + 2, -1);
	_lineSink->UpdateScrollBars ();
}

void ScrollViewPort::OnSplitParagraph (int paraNo, EditParagraph const * firstPara, EditParagraph const * secondPara)
{
	int line = paraNo -_scrollPos._docPara;
	_lineSink->InvalidateLines (line, 2);
	_lineSink->ShiftLines (line + 1, 1);
	_lineSink->UpdateScrollBars ();
}

bool ScrollViewPort::GetLineBoundaries (int lineNo, EditParagraph const * para, 
										int & paraOffsetBegin, int & paraOffsetEnd) const
{
	if (para == 0)
		return false;
	int visCol = 0;
	int visLine;
	paraOffsetBegin = ColWinToDocAdjust (visCol, visLine, para);
	paraOffsetEnd = std::min (para->Len (), paraOffsetBegin + _visCols);
	return true;
}

void ScrollViewPort::OnDelete (int startParaNo, int endParaNo, EditParagraph const * newPara)
{
	if (newPara != 0)
		UpdateMaxParagraphLen (newPara);
	int startPaintLine = startParaNo - _scrollPos._docPara;
	_lineSink->InvalidateLines (startPaintLine, _visLines - startPaintLine + 1);
	_lineSink->UpdateScrollBars ();
}

void ScrollViewPort::InsertParagraphs (int paraNo, std::vector<EditParagraph const *> newParagraphs)
{
	std::vector<EditParagraph const *>::iterator itPara;
	for (itPara = newParagraphs.begin (); itPara != newParagraphs.end (); itPara++)
		UpdateMaxParagraphLen (*itPara);
	int startPaintLine = paraNo - _scrollPos._docPara;
	_lineSink->InvalidateLines (startPaintLine, _visLines - startPaintLine + 1);
	_lineSink->UpdateScrollBars ();
}

void ScrollViewPort::InsertParagraph (int paraNo, EditParagraph const * newPara)
{	
	UpdateMaxParagraphLen (newPara);
	int startPaintLine = paraNo - _scrollPos._docPara;
	_lineSink->InvalidateLines (startPaintLine, _visLines - startPaintLine + 1);
	_lineSink->UpdateScrollBars ();
}

bool ScrollViewPort::AdjustDocPos (EditParagraph const * currPara)
{
	int visCol = ColDocToWin (_docCol, currPara);
	return visCol >= 0 && visCol < _visCols;
}

void ScrollViewPort::OnChangeSelection (int startParaNo, int startOffset, int lastParaNo, int lastOffset)
{
	if (startParaNo == lastParaNo)
	{
		_lineSink->RefreshLineSegment (startParaNo, startOffset, lastOffset);
		return;
	}

	int visLineStart = startParaNo -_scrollPos._docPara;
	int visLineLast = lastParaNo - _scrollPos._docPara;
	int startPaintLine = std::min (visLineStart, visLineLast);
	startPaintLine = std::max (startPaintLine, 0);
	int lastPaintLine = std::max (visLineStart, visLineLast);
	lastPaintLine = std::min (lastPaintLine, _visLines);
	_lineSink->InvalidateLines (startPaintLine, lastPaintLine - startPaintLine + 1);
}

void ScrollViewPort::GetLine (int visLine, int & paraOffsetBegin, int & paraOffsetEnd) const
{
	paraOffsetBegin = 0;
    paraOffsetEnd = PARA_END;
}

//----------------------------------------------------------

// BreakViewPort 
BreakViewPort::BreakViewPort (const ViewPort * otherPort, const Document & doc)
	: ViewPort (*otherPort)
{
	_scrollPos._horScrollPos = 0;
	_lineScrollPos = 0;
	FormatDoc (doc);
	if (_lines.empty ())
		return;

	_lineScrollPos = ParaToLine (_scrollPos._docPara, _scrollPos._docParaOffset);	
	Assert (_lineScrollPos >= 0 && static_cast<unsigned int> (_lineScrollPos) -_lines.size () > 0);
	_scrollPos._docParaOffset = _lines [_lineScrollPos]._paraOffset;
	EditParagraph const * currPara = doc.GetParagraph (_docPara);	
	AdjustDocPos (currPara);
}

// lines

int BreakViewPort::ParaToLine (int para, int paraOffset)  
{
	if (_lines.empty ())
		return 0;
	VisLine line (para, paraOffset);	
	std::vector <VisLine>::iterator itUpp = std::upper_bound (_lines.begin (), _lines.end (), line);
	if (itUpp == _lines.end ())
		return  _lines.size () - 1;
	else
	{
		Assert (itUpp - _lines.begin ()  > 0);
		return  (itUpp - _lines.begin ()) - 1;
	}
}

// line : <paraOffsetBegin, paraOffsetEnd)
bool BreakViewPort::GetLine (EditParagraph const * para, int paraOffsetBegin, int & paraOffsetEnd)const
{
	if (para == 0)
		return false;
	 Assert (paraOffsetBegin >= 0);
	const int len = para->Len ();
	paraOffsetEnd = paraOffsetBegin;
	if (_visCols <= 0)
		return false;
	//revisit : BreakViewPort is unable to settle if the last column is fully visible and 
	//assumes it is not :
	int fullVisCols = std::max (_visCols - 1, 1);

	// find the first invisible paraOffsetEnd (if exist) :	
	for (int nextCol = 1; nextCol <= fullVisCols; nextCol++)
	{
		if (paraOffsetEnd >= len)
			break;
		if ((*para) [paraOffsetEnd] == '\t')
			nextCol = _tabber.SkipTab (nextCol);
		if (nextCol <= fullVisCols) 
			paraOffsetEnd++;
	}
	if (paraOffsetEnd == len) 
		return false;
	
	// now finding good place (space) for breking
	int rowOffsetEnd = paraOffsetEnd; 
	if (!para->IsSpace (paraOffsetEnd) && !para->IsSpace (paraOffsetEnd - 1))
	{
		do 
		{
			paraOffsetEnd--;
		} while (paraOffsetEnd > paraOffsetBegin && !para->IsSpace (paraOffsetEnd -1));
	}
	if (paraOffsetEnd == paraOffsetBegin)
		paraOffsetEnd = rowOffsetEnd;//if not exist good place for braking, we breaking by force
	if (paraOffsetEnd == len) 
		return false;

	if (paraOffsetBegin == paraOffsetEnd)
	{
		//	Bug #687 - we must consume at least one character
		++paraOffsetEnd;
	}

	return true;
}

int BreakViewPort::LineToPara (int line) const
{
	if (_lines.empty ())
		return 0;
	if (line <= 0)
		return 0;
	if (static_cast<unsigned int> (line) >= _lines.size ())
		return _lines [_lines.size () - 1]._para;
	
	return _lines [line]._para;
}

int BreakViewPort::LineWinToPara (int visLine ,int & offset) const
{
	offset = 0;
	if (_lines.empty ())
		return 0;
	
	int line = _lineScrollPos + visLine;
	if (line <= 0)
		return 0;
	if (static_cast<unsigned int> (line)  >=  _lines.size ())
		line = _lines.size () - 1;
	offset = _lines [line]._paraOffset;
	return _lines [line]._para;
}

int BreakViewPort::ParaToWin (int para, int paraOffset)
{
	return ParaToLine (para ,paraOffset) - _lineScrollPos;
}

int BreakViewPort::ParaToWinClip (int para, int offset) 
{
	int line = ParaToWin (para ,offset);
	if (line < 0 || line >= _visLines)
		return -1;
	return line;
}

// columns

int BreakViewPort::ColDocToWin (int offset, EditParagraph const * para) const
{
	if (para == 0)
		return 0;
	if (offset > para->Len ())
		offset = para->Len ();
	//fist find line with this offset
	bool moreLines;
	int paraOffsetBegin = 0;
	int paraOffsetEnd;
	do
	{
		moreLines = GetLine (para, paraOffsetBegin, paraOffsetEnd);		
		if (offset < paraOffsetEnd || paraOffsetEnd == para->Len ())
			break;
		paraOffsetBegin = paraOffsetEnd;		
	} while (moreLines);

     // now find visCol
	int expandCol = _tabber.ExpandTabs (para, paraOffsetBegin, offset - paraOffsetBegin, 0);
	return expandCol;
}

int BreakViewPort::SetDocCol (int docCol)
{
	Assert (_scrollPos._horScrollPos == 0);
	return 0;
}

// If visCol falls in the middle of a tab, stop at the tab.
// If it falls beyond the end of line, stop at the end of line.
// Adjust visCol to reflect the actual stop position
int BreakViewPort::ColWinToDocAdjust (int & visCol, int & visLine, EditParagraph const * para)  const 
{
	if (para == 0 || _lines.size () == 0)
		return 0;
	int line = visLine + _lineScrollPos;
	if (static_cast<unsigned int> (line) >= _lines.size ())
		line = _lines.size () - 1;

	int paraOffsetBegin = _lines [line]._paraOffset;
	int paraOffsetEnd = para->Len ();	
	if (static_cast<unsigned int> (line + 1) < _lines.size () 
		&&  _lines [line]._para == _lines [line + 1]._para)
	{
		//if exist the next line in this para
		paraOffsetEnd = _lines [line + 1]._paraOffset;
	}

	// Start expanding tabs from the beginning of the line until
	// 1. end of line is reached, or
	// 2. one more move would overshoot visCol
	
	int expTargetCol = visCol;
	// offset within paragraph corresponds to expandCol on screen
	int expandCol = 0;
	int offset;
	for (offset = paraOffsetBegin; offset < paraOffsetEnd; offset++)
	{
		int nextCol = expandCol + 1;
		if ((*para) [offset] == '\t')
		{
			int newCol  = _tabber.SkipTab (nextCol);
			nextCol = newCol;
		}	

		if (nextCol > expTargetCol)			
			break; // overshoot
		expandCol = nextCol;
	}

	// Adjust the visible column and line    
	visCol = expandCol;
	if (offset == paraOffsetEnd && offset != para->Len ())
	{   // if end of the line but not end of the paragraph 				
		visCol = 0;
		visLine++;
	}
	return offset;
}

int BreakViewPort::GetDocAnchorCol (EditParagraph const * curPara, int line) const
{
	if (_lines.empty ())
		return 0;
	Assert (line >= 0 && _lines.size () > static_cast<unsigned int> (line));
	int visLine = line - _lineScrollPos;
	int anchorTmp = _anchorCol;
	return ColWinToDocAdjust (anchorTmp, visLine, curPara);		
}

// navigation
int BreakViewPort::GetDocLines (Document const & doc) const
{
	return _lines.size ();
}

int BreakViewPort::SetScrollPosition (EditParagraph const * para, int line)
{
	if (! _lines.empty ())
	{	
		Assert (line >= 0 && _lines.size () > static_cast<unsigned int> (line)); 
		_scrollPos._docParaOffset = _lines [line]._paraOffset;
		_scrollPos._docPara = _lines [line]._para;
	}
	int oldLine = _lineScrollPos;
	_lineScrollPos = line;
	return line - oldLine;
}

int BreakViewPort::Up ()
{
	int line = ParaToLine (_docPara, _docCol);
	line--;
    if (line == -1)
        line = 0;
	_docPara = LineToPara (line);
    return line;
}

int BreakViewPort::Down (int docLen)
{
	if (_lines.empty ())
		return 0;
	int line = ParaToLine (_docPara, _docCol);
	line++;
    if (_lines.size () <= static_cast<unsigned int> (line))
        line = _lines.size () - 1;
	_docPara = LineToPara (line);
    return line;
}

int BreakViewPort::PageDown (int docLen)
{
	if (_lines.empty ())
		return 0;
	int line = ParaToLine (_docPara, _docCol);
	line += (_visLines - 1);
	if (_lines.size () <= static_cast<unsigned int> (line))
        line = _lines.size () - 1;
	_docPara = LineToPara (line);
	return line;
}

int BreakViewPort::PageUp ()
{
	int line = ParaToLine (_docPara, _docCol);
	line -= (_visLines - 1);
	if (line < 0)
        line = 0;
	_docPara = LineToPara (line);
	return line;
}

// formating
void BreakViewPort::SetSize (int visCols, int visLines, const Document & doc)
{
	if (visLines <= 0 || visCols <= 0)
		return ;
	_visLines = visLines;
	if (visCols == _visCols)
		return ;
	_visCols = visCols;
	FormatDoc (doc); 
	if (_lines.empty ())
		return ;

	_lineScrollPos = ParaToLine (_scrollPos._docPara, _scrollPos._docParaOffset);	
	Assert (_lineScrollPos >= 0 && static_cast<unsigned int> (_lineScrollPos) < _lines.size ());
	_scrollPos._docParaOffset = _lines [_lineScrollPos]._paraOffset;
	EditParagraph const * currPara = doc.GetParagraph (_docPara);	
	AdjustDocPos (currPara);
}

void BreakViewPort::FormatDoc (const Document & doc)
{
	_lines.clear ();
	int paraCount = doc.GetParagraphCount ();	
	for (int i = 0; i < paraCount; i++)
	{
		int paraOffsetBegin = 0;
		int paraOffsetEnd = 0;
		bool moreLines;
		EditParagraph  * para = doc.GetParagraph (i);
		do
		{
			_lines.push_back (VisLine (i, paraOffsetBegin));
			moreLines = GetLine (para, paraOffsetBegin, paraOffsetEnd);			
			paraOffsetBegin = paraOffsetEnd;			
		} while (moreLines);
	}	
}

void BreakViewPort::NewDoc (const Document & doc)
{
	FormatDoc (doc);
	ViewPort::AdjustDocPos (doc);
}

void BreakViewPort::GoToDocBegin ()
{
	ViewPort::GoToDocBegin ();	
	_lineScrollPos = 0;
}

void BreakViewPort::OnParaLenChange (DocPosition const & pos, EditParagraph const * para, int deltaLen)
{
	int paraNo = pos.ParaNo ();
	int offsetBegin = pos.ParaOffset ();
	unsigned int oldLinesSize = _lines.size ();
	int startPaintLine = ParaToLine (paraNo, offsetBegin);
    int endPaintLine = startPaintLine;
	Assert (startPaintLine == 0 
		    || (startPaintLine >= 0 && static_cast<unsigned int> (startPaintLine) < _lines.size ()));
	std::vector<VisLine>::iterator itCurr = _lines.begin () + startPaintLine;
	if (_lines.empty ()) // edit at begin
		itCurr = _lines.insert (itCurr, VisLine (0, 0));
    // first special case :
	// we check whether previous line is changed
	int paraOffsetBegin = 0;
	if (itCurr->_paraOffset != 0)
	{	    
		Assert (itCurr != _lines.begin ());
		paraOffsetBegin = (itCurr - 1)->_paraOffset;
		int paraOffsetEnd;
		if (GetLine (para, paraOffsetBegin, paraOffsetEnd))
		{
			paraOffsetBegin = paraOffsetEnd;
			if (paraOffsetBegin != itCurr->_paraOffset)
			{			
				itCurr->_paraOffset = paraOffsetBegin;
				startPaintLine--;
			}
		}
		else 
		{
			startPaintLine--;			
			while (itCurr != _lines.end () && itCurr->_para == paraNo)
			{
				itCurr = _lines.erase (itCurr);
				endPaintLine++;
			}
			RefreshLines (startPaintLine, endPaintLine,  _lines.size () - oldLinesSize);
			return;
		}
	}

	// now we continue checking from current line
	int paraOffsetEnd;
	itCurr++;
	endPaintLine++;
	while (GetLine (para, paraOffsetBegin, paraOffsetEnd))// while more lines
	{
		paraOffsetBegin = paraOffsetEnd;
		// If next lines  no neded repaint , we must change only their parametrs :					
		if (itCurr != _lines.end () &&  paraOffsetEnd == itCurr->_paraOffset + deltaLen)
		{ 
			while (itCurr != _lines.end () && itCurr->_para == paraNo)
			{
				itCurr->_paraOffset += deltaLen;
				itCurr++;
			}
			break;
		}
		//otherwise , if line exist
		if (itCurr != _lines.end () && itCurr->_para == paraNo)
			itCurr->_paraOffset = paraOffsetBegin;
		else // if is new line
			itCurr = _lines.insert (itCurr, VisLine (paraNo, paraOffsetBegin));
			
		endPaintLine++;
		itCurr++;		
	}

    // maybe one on more lines has  disppeared :
	while (itCurr != _lines.end () && itCurr->_para == paraNo)
		itCurr = _lines.erase (itCurr);				

	// sink notificaion : repaint , scroll and update scrollbars
	RefreshLines (startPaintLine, endPaintLine,  _lines.size () - oldLinesSize);
}

void BreakViewPort::OnStyleChanged (int paraNo)
{
	int startPaint = std::max (ParaToWin (paraNo, 0), 0);
	int endPaint =  std::min (ParaToWin (paraNo + 1, 0) - 1, _visLines);
	if (endPaint >= 0 && startPaint <= _visLines)
		_lineSink->InvalidateLines (startPaint, endPaint - startPaint + 1);
}

void BreakViewPort::OnMergeParagraphs (int startParaNo, EditParagraph const * newPara)
{
	unsigned int oldLinesSize = _lines.size ();
	// Merge two paragraphs into newPara: startParaNo and startParaNo + 1
	// All paragraphs preceding startParaNo have valid lines 
	// Old startParaNo has the same lines as the first part of newPara so we can
	// start breaking from tne last line  startParaNo
	// All lines startParaNo + 1 are invalid , we must change or erase them
	// All paragraphs > startParaNo + 1 have "almost" valid  lines - 
	// we must only decrease _para
	
	// find last line of startParaNo pargraph 
	int startPaintLine = ParaToLine (startParaNo + 1, 0);
	Assert (startPaintLine > 0 && static_cast<unsigned int> (startPaintLine) < _lines.size ());
    startPaintLine--;	
	int endPaintLine = startPaintLine;
		
	// this line is valid
	int paraOffsetBegin = _lines [endPaintLine]._paraOffset;	
	int paraOffsetEnd;
	// break newPara starting from paraOffsetBegin 
	while (GetLine (newPara, paraOffsetBegin, paraOffsetEnd))
	{
		endPaintLine++;
		paraOffsetBegin = paraOffsetEnd;
		Assert (static_cast<unsigned int> (endPaintLine) < _lines.size ());
		if (_lines [endPaintLine]._para == startParaNo + 1)
			_lines [endPaintLine] = VisLine (startParaNo, paraOffsetBegin);
		else 
			_lines.insert (_lines.begin () + endPaintLine, VisLine (startParaNo, paraOffsetBegin));
	}

	Assert (static_cast<unsigned int> (endPaintLine) < _lines.size ());
	std::vector <VisLine>::iterator itLine = _lines.begin () + endPaintLine;
	itLine++;
    // Now there may be at most two invalid lines (pointed by itLine)
	while (itLine != _lines.end () && itLine->_para == startParaNo + 1)			    
	   itLine = _lines.erase (itLine);
	
	// now itLine pointed to the beginning of "almost" valid lines: we are only to update _para
	Assert (itLine == _lines.end () || itLine->_para == startParaNo + 2);
	while (itLine != _lines.end ())
	{
		itLine->_para--;
		itLine++;
	}
    // sink notificaion : repaint , scroll and update scrollbars
	RefreshLines (startPaintLine, endPaintLine,  _lines.size () - oldLinesSize);
}

void BreakViewPort::RefreshLines (int startPaintLine, int endPaintLine, int deltaLines)
{
	startPaintLine -= _lineScrollPos;
	endPaintLine  -= _lineScrollPos;
	if (endPaintLine >= 0 && startPaintLine <= _visLines)
	{
		startPaintLine = std::max (startPaintLine, 0);
		endPaintLine = std::min (endPaintLine,_visLines); 
		_lineSink->InvalidateLines (startPaintLine, endPaintLine - startPaintLine + 1);
	}
	if (deltaLines != 0)
	{
		_lineSink->ShiftLines (endPaintLine + 1, deltaLines);
		_lineSink->UpdateScrollBars ();
	}
}

void BreakViewPort::OnSplitParagraph (int paraNo, EditParagraph const * firstPara, 
									  EditParagraph const * secondPara)
{
	unsigned int oldLinesSize = _lines.size ();		
	int startPaintLine = ParaToLine (paraNo, firstPara->Len ()); // last line of first para
	int endPaintLine = startPaintLine; // this variable always points to the last painted line
	
	// Paragraph pointed by paraNo has been split  in two new paragraphs.
	// The first paragraph  usually has valid breaking , but there is a special case - 
	// the (old) last line of first para can dispeared
	// For this reason we  must start breaking since firstPara in startPaintLine - 1 line.

	// firstPara :
	Assert (startPaintLine >= 0 && static_cast<unsigned int> (startPaintLine) < _lines.size ());
	if (_lines [startPaintLine]._paraOffset > 0)
	{	    
		Assert (startPaintLine > 0);
		int paraOffsetBegin = _lines [startPaintLine - 1]._paraOffset;
		int paraOffsetEnd;
		if (! GetLine (firstPara, paraOffsetBegin, paraOffsetEnd)) // one line has dispeared
		{
			startPaintLine--;
			endPaintLine--;			
		}
	}
	
	// Now secondPara:
	int paraOffsetBegin = 0;
	bool moreLines;
	do
	{
		endPaintLine++;
		Assert (static_cast<unsigned int> (endPaintLine) <= _lines.size ());
		int paraOffsetEnd;
		moreLines = GetLine (secondPara, paraOffsetBegin, paraOffsetEnd);
		
		if (static_cast<unsigned int> (endPaintLine) < _lines.size () 
			&& _lines [endPaintLine]._para == paraNo) // current line (from _lines) is invalid
			                                          // we can overwrite it
			_lines [endPaintLine] = VisLine (paraNo + 1, paraOffsetBegin);
		else
			_lines.insert (_lines.begin () + endPaintLine, VisLine (paraNo + 1, paraOffsetBegin));
		paraOffsetBegin = paraOffsetEnd;
	} while (moreLines);

	Assert (static_cast<unsigned int> (endPaintLine) < _lines.size ()); 
	std::vector<VisLine>::iterator it = _lines.begin () + endPaintLine + 1;

	// The rest of the lines :
	// 1.The number of lines from two new paragraphs  can be less than the number of lines 
	//  from "parent" paragraph.In this case we must remove (one) invalid line.
	if (it != _lines.end () && it->_para == paraNo)
		it = _lines.erase (it);

	// 2.Old paraNo + 1 and following paragraphs only increase VisLine._para
    Assert ( it == _lines.end () || it->_para == paraNo + 1); // old paraNo + 1 	
	while (it != _lines.end ())
	{
		it->_para++;
		it++;
	}
	// sink notificaion : repaint , scroll and update scrollbars
	RefreshLines (startPaintLine, endPaintLine,  _lines.size () - oldLinesSize);
}

int	BreakViewPort::LineWinToPara (int visLine) const
{
	int line = visLine + _lineScrollPos;
	if (line >= 0 && _lines.size () > 0)
	{
		if (static_cast<unsigned int> (line) < _lines.size ())
			return _lines [line]._para;
		else
			return _lines [_lines.size () - 1]._para;
	}
		
	return 0;
}

bool BreakViewPort::GetLineBoundaries (int visLine, EditParagraph const * para, 
									   int & paraOffsetBegin, int & paraOffsetEnd) const
{
	int line = visLine + _lineScrollPos;
	if (line >= 0 && static_cast<unsigned int> (line) < _lines.size ())
	{
		// if line exist
		paraOffsetBegin = _lines [line]._paraOffset;
		if (static_cast<unsigned int> (line + 1) < _lines.size () 
			&& _lines [line + 1]._para == _lines [line]._para)
		{
			paraOffsetEnd = _lines [line + 1]._paraOffset;
		}
		else paraOffsetEnd = para->Len ();
		return true;
	}
	return false;
}

void BreakViewPort::OnDelete (int startParaNo, int endParaNo, EditParagraph const * newPara)
{
	// < startParaNo, endParaNo> - paragraphs for delete
	// newPara - newParagraph for breaking.
	unsigned int oldLinesSize = _lines.size ();
	int offset = 0;
	int beginLine = ParaToLine (startParaNo, offset);
	int endLine =  ParaToLine (endParaNo + 1, offset);
	// <beginLine, endLine) - invalid lines : for overwrite or  erase
	if (_lines [endLine]._para != endParaNo + 1) // if endLine ist the last line
		endLine = _lines.size ();
	int deltaPara = endParaNo - startParaNo + 1;
	if (newPara != 0)
	{
        deltaPara--;
		// break newPara
		int paraOffsetBegin = 0;
		bool moreLines;
		do
		{
			int paraOffsetEnd;
			moreLines = GetLine (newPara, paraOffsetBegin, paraOffsetEnd);
			
			if (static_cast<unsigned int> (beginLine) < _lines.size ()
				&& _lines [beginLine]._para <= endParaNo) // current line (from _lines) is invalid
														  // we can overwrite it
				_lines [beginLine] = VisLine (startParaNo, paraOffsetBegin);
			else
			{
				_lines.insert (_lines.begin () + beginLine, VisLine (startParaNo, paraOffsetBegin));
				endLine++;
			}
           
			paraOffsetBegin = paraOffsetEnd;
			beginLine++;
		} while (moreLines);
	}

    // Now erase the rest invalid lines
		
	_lines.erase (_lines.begin () + beginLine, _lines.begin () + endLine);
	std::vector<VisLine>::iterator it = _lines.begin () + beginLine;
	// Following paragraphs only change VisLine._para
	if (deltaPara != 0)
	{
		while (it != _lines.end ())
		{
			it->_para -= deltaPara;
			it++;
		}
	}

    // sink : repaint,update scrollbars
	int beginPaintLine = ParaToWin (startParaNo, 0);;
	_lineSink->InvalidateLines (beginPaintLine, _visLines - beginPaintLine + 1);

	if (oldLinesSize != _lines.size ())
		_lineSink->UpdateScrollBars ();
}

void BreakViewPort::InsertParagraphs (int paraNo, std::vector<EditParagraph const *> newParagraphs)
{
	int deltaPara = newParagraphs.size ();
    // break newParagraphs
	std::vector<VisLine> newLines;
	std::vector<EditParagraph const *>::iterator itPara;
	int currParaNo = paraNo;
	for (itPara = newParagraphs.begin (); itPara != newParagraphs.end (); itPara++)
	{
		int paraOffsetBegin = 0;
		bool moreLines;
		EditParagraph  const * para = *itPara;
		do
		{			
			newLines.push_back (VisLine (currParaNo, paraOffsetBegin));
			int paraOffsetEnd;
			moreLines = GetLine (para, paraOffsetBegin, paraOffsetEnd);			
			paraOffsetBegin = paraOffsetEnd;			
		} while (moreLines);
		currParaNo++;
	}

	int currLine = ParaToLine (paraNo, 0);
	// insert new lines (from newParagraphs)
	_lines.insert (_lines.begin () + currLine, newLines.begin (), newLines.end ());

	// update the rest lines
	 currLine += newLines.size ();	
	std::vector<VisLine>::iterator itLines = _lines.begin () + currLine;	 
	while (itLines != _lines.end ())
	{
		itLines->_para += deltaPara;
		itLines++;
	}

	// sink notificaion : repaint and update scrollbars
    int beginPaintLine = ParaToWin (paraNo, 0);;
	_lineSink->InvalidateLines (beginPaintLine, _visLines - beginPaintLine + 1);
	_lineSink->UpdateScrollBars ();
}

void BreakViewPort::InsertParagraph (int paraNo, EditParagraph const * newPara)
{	
    // break new Para
	std::vector<VisLine> newLines;
	int paraOffsetBegin = 0;
	bool moreLines;	
	do
	{			
		newLines.push_back (VisLine (paraNo, paraOffsetBegin));
		int paraOffsetEnd;
		moreLines = GetLine (newPara, paraOffsetBegin, paraOffsetEnd);			
		paraOffsetBegin = paraOffsetEnd;			
	} while (moreLines);

	// insert new lines (from new paragraph)
	// first find the place for insertion
	VisLine line (paraNo, 0);	
	std::vector <VisLine>::iterator itChanges = std::lower_bound (_lines.begin (), _lines.end (), line);
	int idxChange = itChanges - _lines.begin ();

	_lines.insert (_lines.begin () + idxChange, newLines.begin (), newLines.end ());

	// update the rest lines
	idxChange += newLines.size ();
	itChanges = _lines.begin () + idxChange;
	while (itChanges != _lines.end ())
	{
		itChanges->_para++;
		itChanges++;
	}

	// sink notificaion : repaint and update scrollbars
    int beginPaintLine = ParaToWin (paraNo, 0);
	_lineSink->InvalidateLines (beginPaintLine, _visLines - beginPaintLine + 1);
	_lineSink->UpdateScrollBars ();
}

int BreakViewPort::Left (int & currParaNo, EditParagraph const * currPara, EditParagraph const * prevPara)
{
	if (_lines.empty ())
		return 0;
	
    if (_docCol != 0)
	{
		_docCol--;
		AdjustDocPos (currPara);
	}
    else if (_docPara != 0)
	{
		Assert (prevPara != 0);
		_docCol = prevPara->Len ();
		_docPara--;
		currParaNo = _docPara;
		AdjustDocPos (prevPara);
	}       
    return _docCol;
}

int BreakViewPort::Right (int & docPara, EditParagraph const * currPara, int docLen)
{
	if (_lines.empty () || currPara == 0)
		return 0;
    _docCol++;
	Assert (currPara != 0);
    if (_docCol > currPara->Len ())
    { 
		// next paragraph
		_docCol = 0;
        _docPara++;
        if (_docPara > docLen)
		{
            _docPara = docLen;
			_docCol = currPara->Len ();
		}		
    }
	else if (ColDocToWin (_docCol, currPara) >= _visCols)
	{ 
		// special case: if _docCol point to white space beyond the window
		int currLine = ParaToLine (_docPara, _docCol);
		currLine++;
		if (static_cast<unsigned int> (currLine) < _lines.size ())
		{
			_docCol = _lines [currLine]._paraOffset;
			_docPara = _lines [currLine]._para;			
		}
	}
	docPara = _docPara;
    return _docCol;
}

bool BreakViewPort::AdjustDocPos (EditParagraph const * currPara)
{
	int visCols = _visCols - 1;
    if ( ColDocToWin(_docCol, currPara) >= visCols)
	{
		int visLine = ParaToWin (_docPara, _docCol);
		_docCol = ColWinToDocAdjust (visCols, visLine, currPara);
	}
	return true;
}

void BreakViewPort::OnChangeSelection (int startParaNo, int startOffset, int lastParaNo, int lastOffset)
{
	int lineStart;	
	if (startOffset != PARA_END)
		lineStart = ParaToLine (startParaNo, startOffset);
	else 
		lineStart = LastLineInParagraph (startParaNo);
	
	int lineLast;
	if (lastOffset != PARA_END)
		lineLast = ParaToLine (lastParaNo, lastOffset);
	else 
		 lineLast = LastLineInParagraph  (lastParaNo);

	if (lineStart == lineLast)
	{
		_lineSink->RefreshLineSegment (startParaNo, startOffset, lastOffset);
		return;
	}

	int startPaintLine = std::min (lineStart, lineLast);
	int lastPaintLine = std::max (lineStart, lineLast);

	RefreshLines (startPaintLine, lastPaintLine, 0);
}

void BreakViewPort::GetLine (int visLine, int & paraOffsetBegin, int & paraOffsetEnd) const
{
	paraOffsetBegin = 0;
	if (_lines.size () == 0)
	{
		paraOffsetEnd = 0;
		return;
	}
	int lineNo = visLine + _lineScrollPos;
	if (static_cast<unsigned int> (lineNo) >= _lines.size ())
	{
		paraOffsetEnd = PARA_END;
		return;
	}

	paraOffsetBegin = _lines [lineNo]._paraOffset;
	if (static_cast<unsigned int> (lineNo + 1) >= _lines.size ()
		|| _lines [lineNo + 1]._para != _lines [lineNo]._para)
	{
		// if last line in paraagraph
		paraOffsetEnd = PARA_END;
	}
	else
	{
		paraOffsetEnd = _lines [lineNo + 1]._paraOffset;
	}
}

int BreakViewPort::LastLineInParagraph (int paraNo)
{
	int lineNo = ParaToLine (paraNo + 1, 0);
	if (lineNo <= 0)
		return 0;
	Assert (static_cast<unsigned int> (lineNo) < _lines.size ());
	if (_lines [lineNo]._para == paraNo + 1)
		return lineNo - 1;
	else 
		return lineNo;
}
		


	
