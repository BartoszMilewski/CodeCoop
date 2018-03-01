//-----------------------------------
// Reliable Software (c) 2001 -- 2002
//-----------------------------------

#include "precompiled.h"
#include "Parser.h"
#include "UndoSink.h"
#include "Select.h"
#include "EditStyle.h"

#include <Dbg/Assert.h>


UndoParser::UndoParser (EditLog & editLog, UndoSink & undoSink)
	: _editLog (editLog),
	  _undoSink (undoSink),
	  _input (editLog)
{}

void UndoParser::Execute ()
{
	Action ();		
	_editLog.Erase (_input.GetCurrentLogPosition ());
}

void UndoParser::Action ()
{
	_actionName = static_cast <EditLog::Action::Bits> (_input.GetChar ());
	ReadInfo ();
	int numberSimpleActions = _input.GetLong ();
	for (int k = 0; k < numberSimpleActions; ++k)
		SimpleAction ();
}

void UndoParser::ReadInfo ()
{
	_input.GetSmallString (_info);
}

void UndoParser::SimpleAction ()
{
	EditLog::SimpleAction::Bits actionName = 
		static_cast <EditLog::SimpleAction::Bits> (_input.GetChar ());
	switch (actionName)
	{
		case EditLog::SimpleAction::cutFromParagraph :
			CutFromParagraph ();
			break;
		case EditLog::SimpleAction::cutCompleteParagraph :
			CutCompleteParagraph ();
			break;
		case EditLog::SimpleAction::insertCompleteParagraph :
			InsertCompleteParagraph ();
			break;
		case EditLog::SimpleAction::insert :
			Insert ();
			break;
		case EditLog::SimpleAction::splitParagraph :
			SplitParagraph ();
			break;
		case EditLog::SimpleAction::mergeParagraphs :
			MergeParagraphs ();
			break;
		case EditLog::SimpleAction::appendEmptyParagraph :
			AppendEmptyParagraph ();
			break;
		case EditLog::SimpleAction::deleteChar :
			DeleteChar ();
			break;
		case EditLog::SimpleAction::oldSelection :
			OldSelection ();
			break;
		case EditLog::SimpleAction::newSelection :
			NewSelection ();
			break;
		case EditLog::SimpleAction::positionBegin :
			PositionBegin ();
			break;
		case EditLog::SimpleAction::positionEnd :
			PositionEnd ();
			break;
		default:
			Assert (1 == 0);
	}
}

void UndoParser::CutFromParagraph ()
{
	_input.GetString (_simpleActionBuf);
	DocPosition docPos = _input.GetDocPosition ();
	EditStyle style = _input.GetStyle ();
	_undoSink.CutFromParagraph (docPos, _simpleActionBuf, style);
}

void UndoParser::CutCompleteParagraph ()
{
	_input.GetString (_simpleActionBuf);
	int paraNo = _input.GetLong ();
	EditStyle style = _input.GetStyle ();
	_undoSink.CutCompleteParagraph (paraNo, _simpleActionBuf, style);
}

void UndoParser::InsertCompleteParagraph ()
{
	int paraNo = _input.GetLong ();
	_undoSink.InsertCompleteParagraph (paraNo);
}	

void UndoParser::Insert ()
{
	int paraOffsetEnd = _input.GetLong ();
	int paraOffsetBegin = _input.GetLong ();
	int paraNo = _input.GetLong ();
	EditStyle style = _input.GetStyle ();
	Selection::ParaSegment seg (paraNo, paraOffsetBegin, paraOffsetEnd - paraOffsetBegin);
	_undoSink.Insert (seg, style);
}

void UndoParser::SplitParagraph ()
{
	DocPosition docPos = _input.GetDocPosition ();
	EditStyle style = _input.GetStyle ();
	_undoSink.SplitParagraph (docPos, style);
}

void UndoParser::MergeParagraphs ()
{
	DocPosition docPos = _input.GetDocPosition ();
	EditStyle style2 = _input.GetStyle ();
	EditStyle style1 = _input.GetStyle ();
	_undoSink.MergeParagraphs (docPos, style1, style2);
}

void UndoParser::AppendEmptyParagraph ()
{
	int paraNo = _input.GetLong ();
	_undoSink.AppendEmptyParagraph (paraNo);
}

void UndoParser::DeleteChar ()
{
	char c = _input.GetChar ();
	DocPosition docPos = _input.GetDocPosition ();
	EditStyle style = _input.GetStyle ();
	_undoSink.DeleteChar (docPos, c, style);
}

void UndoParser::OldSelection ()
{
	DocPosition docPosEnd = _input.GetDocPosition ();
	DocPosition docPosBegin = _input.GetDocPosition ();
	_selection = Selection::Marker (docPosBegin, docPosEnd);
	_undoSink.NewSelection  (_selection); 
}

void UndoParser::NewSelection  ()
{
	DocPosition docPosEnd = _input.GetDocPosition ();
	DocPosition docPosBegin = _input.GetDocPosition ();
	Selection::Marker selection (docPosBegin, docPosEnd);
	_undoSink.OldSelection (selection); 
}

Selection::Marker & UndoParser::GetSelection ()
{
	return _selection;
}

void UndoParser::PositionEnd ()
{
	DocPosition end = _input.GetDocPosition ();
	_undoSink.BeginAction (_actionName, end);
}

void UndoParser::PositionBegin ()
{
	_docPosition = _input.GetDocPosition ();
	_undoSink.EndAction (_docPosition);
}

DocPosition & UndoParser::GetPosition ()
{
	return _docPosition;	
}
