//
// (c) Reliable Software, 1997, 98, 99, 2000
//
#include "precompiled.h"
#include "Select.h"
#include "Lines.h"
#include "ViewPort.h"

using namespace Selection;

bool Dynamic::IsSelected (int paraNo) const 
{
	if ( paraNo > std::max (_startParaNo, _endParaNo) 
		|| paraNo < std::min (_startParaNo, _endParaNo))
	{
		return false;
	}
	return true;
	
}

bool Dynamic::IsSelected (int paraNo, int &startCol, int &endCol) const 
{
	if (!IsSelected (paraNo))
		return false;

	if (IsParaSelection ())
	{
		startCol = PARA_START;
		endCol = PARA_END;
		return true;
	}


	if (paraNo == _endParaNo && paraNo == _startParaNo)
	{
		// begining and end in the same line
		if (_anchor.Start () < _endCol || _endCol == PARA_END)
		{
			if (_anchor.Start () == PARA_END)
			{
				startCol = _endCol;
				endCol = PARA_END;
			}
			else
			{
				startCol = _anchor.Start ();
				endCol = _endCol;
			}
		}
		else if (_anchor.Start () >= _endCol || _anchor.Start () == PARA_END)
		{
			// invert
			startCol = _endCol;
			endCol = _anchor.Start ();
		}
		
		else // start == end
		{
			startCol= _anchor.Start ();
			endCol = _endCol;
			
		}
	}
	else if (paraNo == _startParaNo)
	{
		if (_endParaNo < paraNo)
		{
			startCol = PARA_START;
			endCol = _anchor.Start ();
		}
		else
		{
			startCol = _anchor.Start ();
			if (startCol == PARA_END)
				startCol = 0;
			endCol = PARA_END;
		}
	}
	else if (paraNo == _endParaNo)
	{
		// Are we extending up or down?
		if (paraNo > _startParaNo)
		{
			startCol = PARA_START;
			endCol = _endCol;
		}
		else
		{
			startCol = _endCol;
			endCol = PARA_END;
		}
	}
	else
	{
		startCol = PARA_START;
		endCol = PARA_END;
	}

	if (paraNo == _startParaNo && !_anchor.IsEmpty ())
		_anchor.Union (startCol, endCol);

	return startCol != endCol;
}

void Dynamic::StartSelect (int paraNo, int col, Win::KeyState flags) 
{
	// MK_CTRL is processed separately
	if (! flags.IsShift ())
	{
		// Starting a New drag selection
		Clear ();
		_startParaNo = paraNo;
		_anchor.Init (col, col);
	}	
	_endCol = col;
	_endParaNo = paraNo;
}


void Dynamic::StartSelectLine (int paraNo, int colBegin, int colEnd, Win::KeyState flags) 
{
	_paraSelection = false;
	if (flags.IsCtrl ())
	{
		//para Selection
		Clear ();
		_paraSelection = true;
		_startParaNo = paraNo;
		_endParaNo = paraNo;
		_endCol = PARA_END;
		_anchor.Init (PARA_START, PARA_END);
		_selSink->OnChangeSelection (paraNo, PARA_START, paraNo, PARA_END);		
		return;
	}

	int oldEndCol = _endCol;
	int oldEndParaNo = _endParaNo;
	
	if (flags.IsShift ())
	{
		_endParaNo = paraNo;
		// extending the selection from previous starting point				
		if ( paraNo < _startParaNo || (paraNo == _startParaNo && colBegin < _anchor.Start () && colBegin != PARA_END))
			_endCol = colBegin;
		else
			_endCol = colEnd;
		_selSink->OnChangeSelection (oldEndParaNo, oldEndCol, _endParaNo, _endCol);
	}
	else
	{
		Clear ();
		_endParaNo = paraNo;
		_startParaNo = paraNo;
		_endCol = colEnd;		
		_anchor.Init (colBegin, colEnd);
		_selSink->OnChangeSelection (_startParaNo, colBegin, _startParaNo, _endCol);
	}	
}

void Dynamic::ContinueSelect (int paraNo, int col) 
{
	if (_endParaNo == paraNo && _endCol == col)
		return;

	int oldEndCol = _endCol;
	int oldEndParaNo = _endParaNo;
	_endParaNo = paraNo;
	_endCol = col;
	_selSink->OnChangeSelection (oldEndParaNo, oldEndCol, _endParaNo, _endCol);
}

void Dynamic::ContinueSelect (int paraNo, int col, int prevParaNo, int prevCol)
{
	int oldEndParaNo = _endParaNo;
	int oldEndCol = _endCol;
	_endParaNo = paraNo;
	_endCol = col;
	if (!_anchor.IsEmpty ())
	{
		// shrink the anchor to start or end
		int startCol;
		if (paraNo < _startParaNo)
		{
			startCol = _anchor.End ();
		}
		else if (paraNo > _startParaNo)
		{
			startCol = _anchor.Start ();
		}
		else if (prevParaNo < _startParaNo)
		{
			startCol = _anchor.End ();
		}
		else if (prevParaNo > _startParaNo)
		{
			startCol = _anchor.Start ();
		}
		else
			startCol = _anchor.AdjustStartCol (prevCol);

		_anchor.Init (startCol, startCol);
	}

	_selSink->OnChangeSelection (oldEndParaNo, oldEndCol, _endParaNo, _endCol);	
}

void Dynamic::ContinueSelectLine (int paraNo, int startCol, int endCol) 
{	
	int oldEndCol = _endCol;
	int oldEndParaNo = _endParaNo;
	_endParaNo = paraNo;
	if (_paraSelection)
	{
		if (oldEndParaNo == _endParaNo)
			return;
		
		if (_endParaNo >_startParaNo)
		{
	     	_endCol = PARA_END;
		}
		else
		{
			_endCol = PARA_START;
		}
	}
	else 
	{   
		// line selection
		if (_endParaNo < _startParaNo || (paraNo == _startParaNo && startCol < _anchor.Start ()))
			_endCol = startCol;
		else
			_endCol = endCol;
	}
	_selSink->OnChangeSelection (oldEndParaNo, oldEndCol, _endParaNo, _endCol);	
}

// returns true if extending down
void Dynamic::EndSelect (int paraNo, int & col) 
{
	ContinueSelect (paraNo, col);

	if (paraNo == _startParaNo && !_anchor.IsEmpty ())
	{
		// adjust col
		if (_anchor.Start () < col && (_anchor.End () > col || _anchor.End () == PARA_END))
			col = _anchor.End ();
	}
}

bool Dynamic::EndSelectLine (int paraNo, int colBegin, int & colEnd)
{
	ContinueSelectLine (paraNo, colBegin, colEnd);
	
	if (_paraSelection)
	{
		_paraSelection = false;
		return paraNo > _startParaNo;
	}

	return paraNo > _startParaNo || colEnd == _endCol;
}

void Dynamic::Clear () 
{
	if (_startParaNo != _endParaNo || _anchor.Start () != _endCol)
	{
		int oldEndParaNo = _endParaNo;
		int oldEndCol = _endCol;
		_endParaNo = _startParaNo;	
		_endCol = _anchor.Start ();
		_selSink->OnChangeSelection (_startParaNo, _anchor.Start (), oldEndParaNo, oldEndCol);
	}
		
	if (!_anchor.IsEmpty ())
	{
		int anchorStart = _anchor.Start ();
		int anchorEnd = _anchor.End ();
		_anchor.Shrink ();
		_selSink->OnChangeSelection (_startParaNo, anchorStart, _startParaNo, anchorEnd);
	}
}

bool Dynamic::IsSelected (int docPara, int paraOffsetBegin, int paraOffsetEnd , int & offsetSelectBegin, int & offsetSelectEnd) const
{
	if ( !IsSelected (docPara, offsetSelectBegin, offsetSelectEnd))
		return false;
	if (offsetSelectBegin >= paraOffsetEnd)
		return false;
	if (offsetSelectEnd == PARA_END)
		offsetSelectEnd = paraOffsetEnd;		
	if (offsetSelectEnd <= paraOffsetBegin)
		return false;

	offsetSelectBegin = std::max (offsetSelectBegin, paraOffsetBegin);
	offsetSelectEnd = std::min (offsetSelectEnd, paraOffsetEnd);	
	return true;
}

bool Dynamic::IsSelected (int paraNo, int col) const
{
	int selColBegin;
	int selColEnd;
	if (!IsSelected (paraNo, selColBegin, selColEnd))
		return false;
	return col >= selColBegin && (col < selColEnd || selColEnd == PARA_END);
}

void Dynamic::CreateSelection (int paraNo, int paraBeginOffset, int paraEndOffset)
{
	if (!IsEmpty ())
		Clear ();
	_startParaNo = paraNo;	
	_endCol = paraEndOffset;
	_endParaNo = paraNo;
	_anchor.Init (paraBeginOffset, paraEndOffset);
	_selSink->OnChangeSelection (paraNo, paraBeginOffset, paraNo, paraEndOffset);
}

void Dynamic::Select (Marker const & selMarker)
{
	int paraNo = selMarker.Start ().ParaNo ();
	int paraOffset = selMarker.Start ().ParaOffset ();
	StartSelect (paraNo, paraOffset, 0);
	paraNo = selMarker.End ().ParaNo ();
	paraOffset = selMarker.End ().ParaOffset ();
	ContinueSelect (paraNo, paraOffset);
}

Marker::Marker (Dynamic const & selection)
	:_selectionBegin (-1, -1),
	 _selectionEnd (-1, -1)
{
	if (selection.IsEmpty ())
		return;
	int startParaNo = std::min (selection._startParaNo, selection._endParaNo);
	int startSelOffset = - 1;
	int endSelOffset = - 1;
	selection.IsSelected (startParaNo, startSelOffset, endSelOffset);
	_selectionBegin = DocPosition (startParaNo, startSelOffset);
	int endParaNo = std::max (selection._startParaNo, selection._endParaNo);
	selection.IsSelected (endParaNo, startSelOffset, endSelOffset);
	_selectionEnd = DocPosition (endParaNo, endSelOffset);
}

ParaSegment Marker::GetParaSegment () const
{
	Assert (IsSegment  ());
	int paraNo = _selectionBegin.ParaNo ();
	int paraOffset = _selectionBegin.ParaOffset ();
	int len  = _selectionEnd.ParaOffset () - paraOffset;
	return ParaSegment (paraNo, paraOffset, len);
}

bool Marker::IsEqual (ParaSegment const & segment) const
{
	if (!IsSegment  ())
		return false;
	if ( !(segment.GetDocPosition () == _selectionBegin))
		return false;
	if (segment.End () != EndOffset ())
		return false;
	return true;
}

void Dynamic::Segment::Shrink ()
{
	 _endCol = _startCol;
}

void Dynamic::Segment::Union (int & startCol, int & endCol) const
{
	if (startCol == PARA_END)
		startCol = _startCol;
	else if (_startCol != PARA_END)
		startCol = std::min (_startCol, startCol);

	if (_endCol == PARA_END)
		endCol = _endCol;
	else if (endCol != PARA_END)
		endCol = std::max (_endCol, endCol); 
		
}

bool Dynamic::Segment::IsEmpty () const
{
	return _startCol == _endCol;
}

void Dynamic::Segment::Init (int startCol, int endCol)
{
	if (endCol == PARA_END)
	{
		_endCol = endCol;
		_startCol = startCol;
	}
	else if (startCol == PARA_END)
	{
		_endCol = startCol;
		_startCol = endCol;
	}
	else
	{
		_startCol = std::min (startCol, endCol);
		_endCol = std::max (startCol, endCol);
	}	
}

int Dynamic::Segment::AdjustStartCol (int prevCol) const
{
	if (_startCol == _endCol)
		return _startCol;
	else if (prevCol >= _endCol || prevCol == PARA_END)
		return _startCol;
	else 
		return _endCol;
}

bool Dynamic::Segment::IsInSegment (int col) const
{
	if (col != PARA_END)
		return col >= _startCol && (col <= _endCol || _endCol == PARA_END);
	return _endCol == PARA_END;
}
