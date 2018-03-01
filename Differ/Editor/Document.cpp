//-----------------------------------
// (c) Reliable Software, 1997 - 2007
//-----------------------------------

#include "precompiled.h"
#include "Document.h"
#include "Lines.h"
#include "EditPane.h"
#include "Mapper.h" // tabber
#include "ClipboardText.h"
#include "EditLog.h"

#include <Dbg/Assert.h>
#include <StringOp.h>

Document::Document (EditLog * editLog)
    : _maxParagraphLen (0), _paraSink (0), _editLog(editLog)
{}

Document::~Document ()
{}

void Document::Init (std::unique_ptr<EditBuf> editBuf)
{
    _paragraphs = std::move(editBuf);
}

int Document::GetDocEndParaPos () const
{
	if (_paragraphs.get () != 0)
	{
		int paragraphCount = _paragraphs->GetCount ();
		return paragraphCount > 0 ? paragraphCount - 1 : 0;
	}
	return 0;
}

int Document::GetParagraphLen (int i) const
{
    EditParagraph const * para = GetParagraph (i);
    if (para == 0)
        return 0;
    else
        return para->Len ();
}

// If line doesn't exist, appends one
EditParagraph * Document::GetParagraphAlways (int i) 
{
	if (i >= GetParagraphCount ())
		AppendParagraph (i);
	return _paragraphs->GetParagraph (i); 
}

int Document::NextChange (int startLine) const
{
    int count = GetParagraphCount ();
	if (count == 0)
		return -1;
	Assert (startLine < GetParagraphCount ());

    int ln = startLine;
    // skip current change
    while (_paragraphs->GetParagraph (ln)->IsChanged ())
    {
        ln++;
        if (ln == count)
        {
            return -1;
        }
    }

    // find next change
    do
    {
        ln++;
        if (ln == count)
        {
            return -1;
        }
    } while (!_paragraphs->GetParagraph (ln)->IsChanged ());
    return ln;
}

int Document::PrevChange (int startLine) const
{
	if (GetParagraphCount () == 0)
		return -1;
	Assert (startLine < GetParagraphCount ());
    int ln = startLine;
    ln--;
    if (ln == -1)
    {
        return ln;
    }

    // skip the gap
    while (!_paragraphs->GetParagraph (ln)->IsChanged ())
    {
        ln--;
        if (ln == -1)
        {
            return ln;
        }
    }

    // find the beginning of change
    while (_paragraphs->GetParagraph (ln)->IsChanged ())
    {
        ln--;
        if (ln == -1)
        {
            return ln;
        }
    }
    ln++;
    return ln;
}

// Editing

void Document::SplitParagraph (DocPosition const & docPos, EditStyle style1, EditStyle style2)
{
	int paraNo = docPos.ParaNo ();
	int paraOffset = docPos.ParaOffset ();
	EditParagraph * para = GetParagraphAlways (paraNo);
	Assert (para != 0);
	EditStyle oldStyle = para->GetStyle ();
	int targetParaNo = para->GetParagraphNo ();
	int len = para->Len ();
	if (paraOffset == 0)
	{
		std::unique_ptr<EditParagraph> newParagraph (new EditParagraph ("", 0, style1, targetParaNo));
		_paragraphs->InsertParagraph (paraNo, newParagraph);
	}
	else if (paraOffset >= len)
	{
		std::unique_ptr<EditParagraph> newParagraph (new EditParagraph ("", 0, style2, targetParaNo));
		_paragraphs->InsertParagraph (paraNo + 1, newParagraph);
	}
	else
	{
		char * buf = para->Buf ();
		para->SetLen (paraOffset);
		para->SetStyle (style1);
		std::unique_ptr<EditParagraph> newParagraph (new EditParagraph (buf + paraOffset, len - paraOffset, style2, targetParaNo));
		_paragraphs->InsertParagraph (paraNo + 1, newParagraph);
	}
	_editLog->SplitParagraph (docPos, oldStyle);
	_paraSink->OnSplitParagraph (paraNo, GetParagraph (paraNo), GetParagraph (paraNo + 1));	
	_paraSink->OnStyleChanged (paraNo);
}

void Document::AppendParagraph (int paraNo)
{
	int curParaNo = 0;
	if (_paragraphs->GetCount () > 0)
	{
		EditParagraph * lastPara = _paragraphs->GetParagraph (_paragraphs->GetCount () - 1);
		curParaNo = lastPara->GetParagraphNo () + 1;
	}

	EditStyle style (EditStyle::chngUser, EditStyle::actInsert);
	while (_paragraphs->GetCount () <= paraNo)
	{
		_editLog-> AppendEmptyParagraph (paraNo);
		_paragraphs->PutLine ("", 0, style, curParaNo);
		_paraSink->InsertParagraph (paraNo,_paragraphs->GetParagraph (curParaNo));
		curParaNo++;
	}		
}

void Document::InsertCompleteParagraphs(int paraNo, EditBuf const & newParagraphs, int targetParaNo)
{
	EditParagraph * para = GetParagraph (paraNo);
	std::vector<EditParagraph const *> newDocParagraphs; // for _paraSink
	EditStyle style (EditStyle::chngUser, EditStyle::actInsert);
	for (int i = 0; i < newParagraphs.GetCount (); i++)
	{
		EditParagraph const * para = newParagraphs.GetParagraph (i);
		std::unique_ptr<EditParagraph> newParagraph (new EditParagraph (para->Buf (), 
										 para->Len (), style, targetParaNo));
		InsertParagraph (paraNo + i, newParagraph);
		_editLog->InsertCompleteParagraph (paraNo + i,  para);
		newDocParagraphs.push_back (GetParagraph (paraNo + i));
	}
	// Notification 
	if (para != 0)
		newDocParagraphs.push_back (para);
    _paraSink->InsertParagraphs (paraNo, newDocParagraphs);
}

void Document::InsertParagraphs (DocPosition const & docPos, EditBuf const & newParagraphs, int targetParaNo)
{
	if (newParagraphs.GetCount () == 0)
		return;
	if (newParagraphs.GetCount () == 1) // only one paragraph
	{	
		if (newParagraphs.IsNewParagraphAtTextEnd ())
			SplitParagraph (docPos);
		EditParagraph const * para = newParagraphs.GetParagraph (0);
		InsertBuf (docPos, para->Buf (), para->Len ());
	}
	else
	{	
		bool isInsertAtEnd = (docPos.ParaNo () == GetDocEndParaPos () &&
							  docPos.ParaOffset () == GetParagraphLen (GetDocEndParaPos ()));

		//	split unless we're at the end of the file with no newline
		if (!isInsertAtEnd || _paragraphs->IsNewParagraphAtTextEnd ())
			SplitParagraph (docPos);

		EditParagraph const * para = newParagraphs.GetParagraph (0);
		InsertBuf (docPos, para->Buf (), para->Len ());
		for (int k = 1; k < newParagraphs.GetCount (); ++k)
		{
			para = newParagraphs.GetParagraph (k);
			InsertParagraph (docPos.ParaNo () + k, para->Buf (), para->Len ());
		}
		if (!isInsertAtEnd && !newParagraphs.IsNewParagraphAtTextEnd ())
			MergeParagraphs (docPos.ParaNo () + newParagraphs.GetCount () - 1);
	}
}

void Document::DeleteChar (DocPosition const & docPos)
{
	int paraNo = docPos.ParaNo ();
	int paraOffset = docPos.ParaOffset ();
	EditParagraph * para = _paragraphs->GetParagraph (paraNo);
	Assert (para != 0 && paraOffset >= 0 && paraOffset < para->Len ());
	char c = para->Buf ()[paraOffset]; 
	para->DeleteChar (paraOffset);
	_paraSink->OnParaLenChange (docPos, para, -1);
	EditStyle style (EditStyle::chngUser, EditStyle::actInsert);
	EditStyle oldStyle = para->GetStyle ();
	if (oldStyle != style)
	{
		para->SetStyle (style);
		_paraSink->OnStyleChanged (paraNo);
	}
	_editLog->DeleteChar (docPos, c, oldStyle);	
}

void Document::InsertChar (DocPosition const & docPos, int c, EditStyle style)
{
	int paraNo = docPos.ParaNo ();
	int paraOffset = docPos.ParaOffset ();
	EditParagraph * para = GetParagraphAlways (paraNo);
	Assert (para != 0);
	EditStyle oldStyle = para->GetStyle ();
	para->InsertChar (paraOffset, c);
	_editLog->InsertChar (docPos, c, oldStyle);
	_paraSink->OnParaLenChange (docPos, para, 1);
	if (oldStyle != style)
	{
		para->SetStyle (style);
		_paraSink->OnStyleChanged (paraNo);
	}
}

bool Document::Find (SearchRequest const * searchRequest, Selection::ParaSegment & inOut)
{
	int paraNo = inOut.ParaNo ();
	EditParagraph * para = _paragraphs->GetParagraph (paraNo);
	Assert (para != 0);
	Finder finder (searchRequest);

	// First search through the current paragraph. If text was not found
	// then search through the rest of the paragraphs 
	if (! finder.FindIn (para->Buf (), para->Len (), inOut.ParaOffset ())) // current paragraph 
	{
		// the rest of the paragraphs :
		if (finder.IsDirectionForward ())
		{
			++paraNo;
			EditBuf::iterator it;		
			it = std::find_if (_paragraphs->ToIter (paraNo), _paragraphs->end (), FindPredicate (finder));
			if (it == _paragraphs->end ()) // not found 
				return false;
			paraNo = _paragraphs->ToIndex (it);
		}
		else
		{ // direction up		
			--paraNo;
			EditBuf::reverse_iterator rit;
			rit = std::find_if (_paragraphs->ToRIter (paraNo),_paragraphs->rend (), FindPredicate (finder));
			if (rit == _paragraphs->rend ()) // not found
				return false;
			paraNo = _paragraphs->ToIndex (rit);
		}
	}

	inOut.SetLen (finder.GetResultLen ());
	inOut.SetParaOffset (finder.GetResultOffset ());
	inOut.SetParaNo (paraNo);
	return true;
}

void Document::MergeParagraphs (int startParaNo, EditStyle style)
{
	Assert (startParaNo < GetParagraphCount ());
	std::string buf;
	EditParagraph * para = _paragraphs->GetParagraph (startParaNo);
	EditStyle oldStyleFirstPara = para->GetStyle ();
	int targetParaNo = para->GetParagraphNo ();
	// Copy merged lines into the buffer :
	int lenFirstPara = para->Len ();
	buf.append (para->Buf (), lenFirstPara);
	para = _paragraphs->GetParagraph (startParaNo + 1);
	EditStyle oldStyleSecondPara (EditStyle::chngUser, EditStyle::actInsert);
	if (para)
	{
		oldStyleSecondPara = para->GetStyle ();
		buf.append (para->Buf (), para->Len ());
		_paragraphs->Delete (startParaNo + 1);	
	}
	_paragraphs->Delete (startParaNo);
	_editLog->MergeParagraphs(DocPosition (startParaNo, lenFirstPara), oldStyleFirstPara, oldStyleSecondPara);

	// Insert merged line :
	std::unique_ptr<EditParagraph> newParagraph (new EditParagraph (buf.c_str (), buf.size (), style, targetParaNo));
	_paragraphs->InsertParagraph (startParaNo, newParagraph);
	// notifications
	_paraSink->OnMergeParagraphs (startParaNo, _paragraphs->GetParagraph (startParaNo));
	_paraSink->OnStyleChanged (startParaNo);

}

void Document::Delete (Selection::Marker const & selection)
{
	if (selection.StartParaNo () != selection.EndParaNo ()) // if more than one paragraph
	{ // we work with the paragraphs in the reverse order, from last to first

		// first the last paragraph:
		if (selection.EndOffset () == PARA_END)
			DeleteParagraph (selection.EndParaNo ());
		else
		{
			Selection::ParaSegment seg (selection.EndParaNo (), 0, selection.EndOffset ()); 
			CutFromParagraph (seg);
		}
		// the midle paragraphs
		for (int k = selection.EndParaNo () - 1; k > selection.StartParaNo (); --k)
			DeleteParagraph (k);

		// the first partagraph
		EditParagraph const * firstPara = GetParagraph (selection.StartParaNo ());
		Selection::ParaSegment seg (selection.Start (), firstPara->Len () - selection.StartOffset ());
		CutFromParagraph (seg);
		// in the end, merging the remains of the first and the last paragraph
		// unless we cut off the end of the document
		if (GetParagraphCount () > selection.StartParaNo () + 1)
			MergeParagraphs (selection.StartParaNo (), GetStyleParaNo (selection.StartParaNo () + 1));		
	}
	else // if only one paragraph  
	{	
		if (selection.EndOffset () != PARA_END)
			CutFromParagraph (selection.GetParaSegment ());
		else
		{
			EditParagraph const * para = GetParagraph (selection.StartParaNo ());
			Selection::ParaSegment seg (selection.Start (), para->Len () - selection.StartOffset ());
			CutFromParagraph (seg);
			if (GetParagraphCount () > selection.StartParaNo () + 1)
				MergeParagraphs (selection.StartParaNo (), GetStyleParaNo (selection.StartParaNo () + 1));
		}
	}
}

void Document::Replace (Selection::ParaSegment const & segment, const std::string & text)
{
	int paraNo = segment.ParaNo ();
	EditParagraph * para = _paragraphs->GetParagraph (paraNo);
	Assert (para != 0);
	int endOffset = segment.End ();
	if (endOffset == PARA_END)
		endOffset = para->Len ();
	const char * buf = para->Buf ();
	int startOffset = segment.ParaOffset ();
	std::string oldText (buf + startOffset, buf + endOffset);
	EditStyle oldStyle = para->GetStyle ();
	_editLog->Replace (segment.GetDocPosition (), oldText, text, oldStyle);
	para->Replace (startOffset, endOffset, text);
	_paraSink->OnParaLenChange(segment.GetDocPosition (), para, startOffset - endOffset + 1);
	
	EditStyle style (EditStyle::chngUser, EditStyle::actInsert);
	if (para->GetStyle () != style)
	{
		para->SetStyle (style);
		_paraSink->OnStyleChanged (paraNo);
	}
}

int Document::ReplaceAll (ReplaceRequest const * request, DocPosition & curPos)
{
	int count = 0;
	ReplaceRequest replaceRequest = *request; // copy of replace request
	replaceRequest.SetDirectionForward (true);
	// start at the beginning of the document :
	Selection::ParaSegment segment (0, 0);
	while (Find (&replaceRequest, segment))
	{
		++count;		
		Replace (segment, replaceRequest.GetSubstitution ());	
		if (segment.ParaNo () == curPos.ParaNo ())
		{
			// calculate new doc position
			int currOffset = curPos.ParaOffset ();
			if (segment.End () < curPos.ParaOffset ())				
				currOffset += replaceRequest.GetSubstitution ().size () - segment.Len ();
			else if (segment.ParaOffset () < currOffset 
				     && replaceRequest.GetSubstitution ().size () < static_cast<unsigned int> (currOffset - segment.ParaOffset ()))
				currOffset = segment.ParaOffset () + replaceRequest.GetSubstitution ().size ();
			curPos.SetParaOffset (currOffset);
		}
		// start the next search from the end of the substitution	
		int startOffset = segment.ParaOffset ();
		segment.SetParaOffset (startOffset + replaceRequest.GetSubstitution ().size ());		
	}
	return count;
}

Document::Finder::Finder (SearchRequest const * req)
		:_serchReq (*req), _findWord (_serchReq.GetFindWord ()), _startOffset (0)
{
	if (!_serchReq.IsMatchCase ())    
		std::transform (_findWord.begin (), _findWord.end (), _findWord.begin (), ::ToLower);
}

bool Document::Finder::FindIn (char const * buf, int lenBuf, int startOffset)
{
	_startOffset = startOffset;
	if (_serchReq.IsDirectionForward ())
		return MatchForward (buf, lenBuf);
	else
		return MatchBackward (buf, lenBuf);	 	
}

bool Document::Finder::MatchForward (char const * buf, int lenBuf) 
{
	int limit = lenBuf - _findWord.size () + 1;
	if (_serchReq.IsMatchCase ())
	{
		for ( ; _startOffset < limit; ++_startOffset)
			if (MatchSensitive (buf, lenBuf))
				return true;
	}
	else
	{
		for ( ; _startOffset < limit; ++_startOffset)
			if (MatchInsensitive (buf, lenBuf))
				return true;
	}			
	return false;
}

bool Document::Finder::MatchBackward (char const * buf, int lenBuf)
{
	_startOffset -= _findWord.size (); // Make sure the match doesn't overlap the cursor (startOffset)
	if (_serchReq.IsMatchCase ())
	{
		for ( ; _startOffset >= 0; --_startOffset)
			if (MatchSensitive (buf, lenBuf))
				return true;
	}
	else
	{
		for ( ; _startOffset >= 0; --_startOffset)
			if (MatchInsensitive (buf, lenBuf))				
				return true;
	}		
	return false;
}

bool Document::Finder::MatchInsensitive (char const * buf, int lenBuf) const
{	
	int wordSize = _findWord.size ();
	Assert (_startOffset >= 0 && _startOffset + wordSize <= lenBuf);	
	for (int k = 0; k < wordSize; ++k)
		if (_findWord [k] != ::ToLower (buf [_startOffset + k]))
			return false;

	if (_serchReq.IsWholeWord ())
		return IsWholeWord (buf, lenBuf);

	return true;	
}

bool Document::Finder::MatchSensitive (char const * buf, int lenBuf) const
{
	int wordSize = _findWord.size ();
	Assert (_startOffset >= 0 && _startOffset + wordSize <= lenBuf);
	for (int k = 0; k < wordSize; ++k)
		if (_findWord[k] != buf [_startOffset + k])
			return false;

	if (_serchReq.IsWholeWord ())
		return IsWholeWord (buf, lenBuf);

	return true;
}

bool Document::Finder::IsWholeWord (char const * buf, int lenBuf) const
{
	int end = _startOffset + _findWord.size ();
	Assert (_startOffset >= 0 && end <= lenBuf);
	return (_startOffset == 0 || ::IsWordBreak (buf [_startOffset -1])) 
			&& (end == lenBuf || ::IsWordBreak (buf [end]));
}

bool Document::FindPredicate::operator () (const EditParagraph * para)
{	
	char const * buf = para->Buf ();
	int lenBuf = para->Len ();
	int startOffset = 0;
	if (!_finder.IsDirectionForward ())
		startOffset = lenBuf;
	return _finder.FindIn (buf, lenBuf, startOffset);
}

void Document::InsertBuf (DocPosition const & docPos, char const * buf, int lenBuf, EditStyle style)
{
	int paraNo = docPos.ParaNo ();
	int paraOffset = docPos.ParaOffset ();
	EditParagraph * para = GetParagraphAlways (paraNo);
	// create the new buf
	std::string bufNewPara (para->Buf (), paraOffset);
	bufNewPara.append (buf, lenBuf);
	bufNewPara.append (para->BufAt (paraOffset), para->Len () - paraOffset);
	EditStyle oldStyle = para->GetStyle ();
	// delete the old paragraph
	_paragraphs->Delete (paraNo);
	// create the new paragraph and insert it 
	std::unique_ptr<EditParagraph> newParagraph (new EditParagraph (bufNewPara.c_str (), bufNewPara.size (), style, docPos.ParaNo ()));
	_paragraphs->InsertParagraph (paraNo, newParagraph);
	// notifications :
	_paraSink->OnParaLenChange (docPos, _paragraphs->GetParagraph (paraNo), lenBuf);
	if (style != oldStyle)
		_paraSink->OnStyleChanged (paraNo);
	Selection::ParaSegment segment (docPos, lenBuf);
	_editLog->Insert (segment, _paragraphs->GetParagraph (paraNo)->BufAt(docPos.ParaOffset ()), oldStyle);
}

void Document::InsertParagraph (int paraNo, char const * buf, int lenBuf, EditStyle style)
{
	// create new paragraph and insert it
	std::unique_ptr<EditParagraph> newParagraph (new EditParagraph (buf, lenBuf, style, paraNo));
	_paragraphs->InsertParagraph (paraNo, newParagraph);
	// notifications :
	_paraSink->InsertParagraph (paraNo, GetParagraph (paraNo));
	_paraSink->OnStyleChanged (paraNo);
	_editLog->InsertCompleteParagraph (paraNo, GetParagraph (paraNo));
}

void Document::CutFromParagraph (Selection::ParaSegment const & segment, EditStyle style)
{
	EditParagraph * para = GetParagraph (segment.ParaNo ());
	// first notify the edit log
	EditStyle oldStyle = para->GetStyle ();
	_editLog->CutFromParagraph (segment, para, oldStyle);
	// create the new buf
	char * buf = para->Buf ();
	std::string newBuf;
	newBuf.append (buf, segment.ParaOffset ());
	newBuf.append (buf + segment.End (), para->Len () - segment.End ());
	// delete the old paragraph
	_paragraphs->Delete (segment.ParaNo ());
	// create the new paragraph and insert it
	std::unique_ptr<EditParagraph> newParagraph (new EditParagraph (newBuf.c_str (), newBuf.size (), style, segment.ParaNo ()));
	_paragraphs->InsertParagraph (segment.ParaNo (), newParagraph);
	// notifications :
	int deltaLen =  -segment.Len ();
	_paraSink->OnParaLenChange (segment.GetDocPosition (), _paragraphs->GetParagraph (segment.ParaNo ()), deltaLen);
	if (style != oldStyle)
		_paraSink->OnStyleChanged (segment.ParaNo ());
}

void Document::DeleteParagraph (int paraNo)
{
	EditParagraph * para = GetParagraph (paraNo);
	_editLog->CutCompleteParagraph (paraNo, para, para->GetStyle ());
	_paragraphs->Delete (paraNo);
	_paraSink->OnDelete (paraNo, paraNo, 0);
}

EditStyle Document::GetStyleParaNo (int paraNo) const
{
	EditParagraph  const * para = GetParagraph (paraNo);
	return para->GetStyle ();
}

void Document::AddTabs (int firstParaNo, int lastParaNo)
{
	char const * buf = "\t";
	const int lenBuf = 1;
	for (int k = firstParaNo; k <= lastParaNo; ++k)
	{
		int pointInsertion = 0;
		const EditParagraph *  para = _paragraphs->GetParagraph (k);
		Assert (para != 0);
		while (pointInsertion < para->Len () && ((*para) [pointInsertion] == '\t' || para->IsSpace (pointInsertion)))
			++pointInsertion;
		InsertBuf (DocPosition (k, pointInsertion),  buf, lenBuf);
	}
}

int Document::DeleteTabOrSpaces (int paraNo, int tabSize)
{
	int lastTabOffset = 0;
	const EditParagraph *  para = _paragraphs->GetParagraph (paraNo);
	Assert (para != 0);
	int deltaLen = 0;
	for (lastTabOffset = 0; lastTabOffset < para->Len (); )
	{
		if ((*para) [lastTabOffset] == '\t')
		{
			++lastTabOffset;
			deltaLen = 1;
		}
		else if (para->IsSpace (lastTabOffset))
		{
			deltaLen = 0;
			while (lastTabOffset < para->Len () && para->IsSpace (lastTabOffset) && deltaLen < tabSize)
			{
				++lastTabOffset;
				++deltaLen;
			}
			if (lastTabOffset < para->Len () && (*para) [lastTabOffset] == '\t' && deltaLen < tabSize)
			{
				++lastTabOffset;
				++deltaLen;
			}
		}
		else 
			break;
	}
	if (deltaLen > 0)
	{
		DocPosition beg (paraNo, lastTabOffset - deltaLen);
		DocPosition end (paraNo, lastTabOffset);
		Selection::Marker sel (beg, end);
		Delete (sel);
	}
	return deltaLen;
}
