//
// Reliable Software (c) 2001
//

#include "precompiled.h"
#include "UndoSink.h"
#include "Document.h"
#include "Select.h"

void UndoSink::BeginAction (EditLog::Action::Bits actionName, DocPosition const & docPos)
{
	_editLog.BeginAction (actionName, docPos, true);
}

void UndoSink::EndAction (DocPosition const & docPos)
{
	_editLog.EndAction (docPos);
}

void UndoSink::CutFromParagraph (DocPosition const & docPos, std::string const & buf, EditStyle style)
{
	_doc.InsertBuf (docPos, buf.c_str (), buf.size (), style); 
}

void UndoSink::CutCompleteParagraph (int paraNo, std::string const & buf, EditStyle style)
{
	_doc.InsertParagraph (paraNo, buf.c_str (), buf.size (), style); 
}

void UndoSink::InsertCompleteParagraph (int paraNo)
{
	_doc.DeleteParagraph (paraNo);
}

void UndoSink::Insert (Selection::ParaSegment const & seg, EditStyle style) 
{
	_doc.CutFromParagraph (seg, style);
}

void UndoSink::SplitParagraph (DocPosition const & docPos, EditStyle style) 
{
	_doc.MergeParagraphs (docPos.ParaNo (), style);
}

void UndoSink::MergeParagraphs (DocPosition const & docPos, EditStyle style1, EditStyle style2) 
{
	_doc.SplitParagraph (docPos, style1, style2);
}

void UndoSink::AppendEmptyParagraph (int paraNo) 
{
	_doc.DeleteParagraph (paraNo);
}

void UndoSink::DeleteChar (DocPosition const & docPos, char c, EditStyle style) 
{	
	_doc.InsertChar (docPos, c, style);
}

void UndoSink::OldSelection (Selection::Marker const & selection)
{
	_editLog.OldSelection (selection);
}

void UndoSink::NewSelection (Selection::Marker const & selection)
{
	_editLog.NewSelection (selection);
}

