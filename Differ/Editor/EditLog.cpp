//
// Reliable Software (c) 1998 - 2002
//
#include "precompiled.h"
#include "EditLog.h"
#include "Lines.h"
#include "DocPosition.h"
#include "Select.h"
#include "EditPane.h"
#include <Dbg/Assert.h>
#include <Dbg/Out.h>


#define DEGUG_EDITLOG  0

#if DEGUG_EDITLOG


const char * ActionNames [] =
{
	"typing",          
	"cut",
	"paste",
	"del",
	"replaceAll",
	"backSpace",
	"newLine",
	"replace",
	"autoIndent",
	"continueTyping",
	"multiParaTabAdd",
	"multiParaTabDel",
	"none"
};

const char * SimpleActionNames [] =
{	
	"cutFromParagraph",
	"cutCompleteParagraph",		
	"insertCompleteParagraph",
	"insert",
	"splitParagraph",
	"mergeParagraphs",
	"appendEmptyParagraph",		
	"deleteChar",			
	"oldSelection,",
	"newSelection",
	"positionBegin",
	"positionEnd",
	"infoBuf"		
};

std::ostream& operator<<(std::ostream& os, EditLog::Action action)
{
	const char * name = ActionNames [action];
	os << name << " " << std::flush;
	return os;
}
	
std::ostream& operator<<(std::ostream& os, EditLog::SimpleAction action)
{
	const char * name = SimpleActionNames [action];
	os << name << " " << std::flush;
	return os;
}

void EditLog::Recorder::WriteLong (long Long)
{
	dbg << Long << " " << std::flush;
	char * pChar  = reinterpret_cast<char*> (& Long);
	for (int k = 0; k < 4; ++k)
		_buf.push_back (pChar [k]);
}

void EditLog::Recorder::WriteText (int lenBuf, char const * text)
{ 
	dbg.write (text, lenBuf);
	dbg << " ";
	for (int k = 0; k < lenBuf; ++k)
		_buf.push_back (text [k]);
	WriteLong (lenBuf);
}

void EditLog::Recorder::WriteSmallText (int lenBuf, char const *  text)
{
	dbg.write (text, lenBuf);
	dbg << " " << lenBuf << " " << std::flush;
	Assert (lenBuf < 256);
	for (int k = 0; k < lenBuf; ++k)
		_buf.push_back (text [k]);
	WriteChar (lenBuf);
}

#else // DEGUG_EDITLOG

std::ostream& operator<<(std::ostream& os, EditLog::SimpleAction action)
{
	return os;
}

void EditLog::Recorder::WriteLong (long Long)
{
	char * pChar  = reinterpret_cast<char*> (& Long);
	for (int k = 0; k < 4; ++k)
		_buf.push_back (pChar [k]);
}

void EditLog::Recorder::WriteText (int lenBuf, char const * text)
{
	for (int k = 0; k < lenBuf; ++k)
		_buf.push_back (text [k]);
	WriteLong (lenBuf);
}

void EditLog::Recorder::WriteSmallText (int lenBuf, char const *  text)
{
	Assert (lenBuf < 256);
	for (int k = 0; k < lenBuf; ++k)
		_buf.push_back (text [k]);
	WriteChar (lenBuf);
}

#endif //DEGUG_EDITLOG


void EditLog::Recorder::WriteStyle (EditStyle const & style)
{
	WriteChar (style.GetAction ());
	WriteChar (style.GetChangeSource ());
}

void EditLog::Recorder::Write (DocPosition const & docPos)
{
	WriteLong (docPos.ParaNo ());
	WriteLong (docPos.ParaOffset ());
}

void EditLog::Recorder::Write (Selection::ParaSegment const & paraSeg)
{
	WriteLong (paraSeg.ParaNo ());
	WriteLong (paraSeg.ParaOffset ());
	WriteLong (paraSeg.End ());
}

void EditLog::Recorder::Erase (SourceIter iter)
{
	// SourceIter it is reverse_iterator
	std::deque<char>::iterator it = iter.base(); 
	_buf.erase (it, _buf.end ()); 
}

char EditLog::Input::GetChar ()
{
	// In the future:
	// If we save editlog on disc we must use here exeption
	Assert (_sourceIter != _sourceEnd);
	return *(_sourceIter++);
}

long EditLog::Input::GetLong ()
{
	long Long;
	char * pChar  = reinterpret_cast<char*> (& Long);
	// the multibytae data must read reverse :
	for (int k = 3; k >= 0; --k)
		pChar [k] = GetChar ();
		
	return Long;
}

void EditLog::Input::GetString (std::string & str)
{	
	int size = GetLong ();
	 str.resize (size);
	// the multibytae data must read reverse :
	for (int k = size - 1; k >= 0; --k)
		str [k] = GetChar ();
}

void EditLog::Input::GetSmallString (std::string & str)
{
	int size = GetChar ();
	 str.resize (size);
	// the multibytae data must read reverse :
	for (int k = size - 1; k >= 0; --k)
		str [k] = GetChar ();
}

DocPosition EditLog::Input::GetDocPosition ()
{
	int offset = GetLong ();
	int paraNo = GetLong ();
	return DocPosition (paraNo, offset);
}

EditStyle EditLog::Input::GetStyle ()
{
	EditStyle::Source source = static_cast <EditStyle::Source> (GetChar ());
	EditStyle::Action action = static_cast <EditStyle::Action> (GetChar ());
	return EditStyle (source, action);
}
	
EditLog::EditLog (EditStateNotificationSink * stateSink)
	:_curAction (Action::none), 
	_countAtomicAction (0), 
	_endLastAction (0),
	_stateSink (stateSink),
	_isEmptyAction (true)
{}

void EditLog::Erase (SourceIter iter) 
{
	_recorder.Erase (iter);
	_endLastAction = _recorder.Size ();
	if (_recorder.Empty ())
	_stateSink->OnChangeState ();
}

void EditLog::Clear () throw ()
{
	_recorder.Clear ();
	_endLastAction = 0;
	_countAtomicAction = 0;
	_stateSink->OnChangeState ();
	_curAction = Action::none;
	_isEmptyAction = true;
}

void EditLog::Flush ()
{
	if (_countAtomicAction == 0)
		return;
	Assert (_curAction != Action::none);
	_countAtomicAction++;
	_recorder.Write (_posActionEnd);
	_recorder.WriteChar (SimpleAction::positionEnd);
	dbg << SimpleAction::positionEnd;
	_recorder.WriteLong (_countAtomicAction);
	_recorder.WriteSmallText (_curActionInfo.size (), _curActionInfo.c_str ());
	dbg << SimpleAction::infoBuf;
	_recorder.WriteChar (_curAction);
#if DEGUG_EDITLOG
	dbg << _curActionName << " " << std::endl << std::flush;
#endif
	
	// reinit variables
	_curActionInfo.clear ();
	_countAtomicAction = 0;
	_curAction = Action::none;
	_endLastAction = _recorder.Size ();
	_isEmptyAction = true;
}

void EditLog::CutFromParagraph (Selection::ParaSegment const & paraSeg, EditParagraph const * para, EditStyle style)
{
	_countAtomicAction++;
	_isEmptyAction = false;
	_recorder.WriteStyle (style);
	_recorder.Write (paraSeg.GetDocPosition ());
	_recorder.WriteText (paraSeg.Len (), para->BufAt (paraSeg.ParaOffset ()));
	_recorder.WriteChar (SimpleAction::cutFromParagraph);
	dbg << SimpleAction::cutFromParagraph;
}

void EditLog::CutCompleteParagraph (int paraNo, EditParagraph const * para, EditStyle style)
{
	_countAtomicAction++;
	_isEmptyAction = false;
	_recorder.WriteStyle (style);
	_recorder.WriteLong (paraNo);
	_recorder.WriteText (para->Len (), para->Buf ());	
	_recorder.WriteChar (SimpleAction::cutCompleteParagraph);
	dbg << SimpleAction::cutCompleteParagraph;
}
	
void EditLog::Insert (Selection::ParaSegment const & paraSeg, char const * buf, EditStyle style)
{
	_countAtomicAction++;
	_isEmptyAction = false;
	MakeInfo (buf, paraSeg.Len ());
	_recorder.WriteStyle (style);
	_recorder.Write (paraSeg);
	_recorder.WriteChar (SimpleAction::insert);
	dbg << SimpleAction::insert;
}

void EditLog::InsertCompleteParagraph (int paraNo, EditParagraph const * para)
{
	_countAtomicAction++;
	_isEmptyAction = false;
	MakeInfo (para->Buf (), para->Len ());
	_recorder.WriteLong (paraNo);
	_recorder.WriteChar (SimpleAction::insertCompleteParagraph);
	dbg << SimpleAction::insertCompleteParagraph;
}

void EditLog::SplitParagraph (DocPosition const & docPos, EditStyle style)
{
	_countAtomicAction++;
	_isEmptyAction = false;
	_recorder.WriteStyle (style);
	_recorder.Write (docPos);
	_recorder.WriteChar (SimpleAction::splitParagraph);
	dbg << SimpleAction::splitParagraph;
}

void EditLog::MergeParagraphs (DocPosition const & docPos, EditStyle style1, EditStyle style2)
{
	_countAtomicAction++;
	_isEmptyAction = false;
	_recorder.WriteStyle (style1);
	_recorder.WriteStyle (style2);
	_recorder.Write (docPos);
	_recorder.WriteChar (SimpleAction::mergeParagraphs);
	dbg << SimpleAction::mergeParagraphs;
}

void EditLog::AppendEmptyParagraph (int paraNo)
{
	_countAtomicAction++;
	_isEmptyAction = false;
	_recorder.WriteLong (paraNo);
	_recorder.WriteChar (SimpleAction::appendEmptyParagraph);
	dbg << SimpleAction::appendEmptyParagraph;
}

void EditLog::Replace (DocPosition const & docPos, const std::string & oldStr, const std::string & newStr, EditStyle style)
{
	// Cut and paste
	_countAtomicAction++;
	_isEmptyAction = false;
	_recorder.WriteStyle (style);
	_recorder.Write (docPos);	
	_recorder.WriteText (oldStr.size (), oldStr.c_str ());
	_recorder.WriteChar (SimpleAction::cutFromParagraph);
	dbg << SimpleAction::cutFromParagraph;

	_countAtomicAction++;
	_recorder.WriteStyle (style);
	_recorder.Write (docPos);	
	_recorder.WriteLong (docPos.ParaOffset () + newStr.size ());
	_recorder.WriteChar (SimpleAction::insert);
	dbg << SimpleAction::insert;
}

void EditLog::DeleteChar (DocPosition const & docPos, char c, EditStyle style)
{
	_countAtomicAction++;
	_isEmptyAction = false;
	_recorder.WriteStyle (style);
	_recorder.Write (docPos);
	_recorder.WriteChar (c);
	_recorder.WriteChar (SimpleAction::deleteChar);
	dbg << SimpleAction::deleteChar;
}

void EditLog::OldSelection (Selection::Marker const & selection)
{
	_countAtomicAction++;
	_recorder.Write (selection.Start ());
	_recorder.Write (selection.End ());
	_recorder.WriteChar (SimpleAction::oldSelection);
	dbg << SimpleAction::oldSelection;
}

void EditLog::NewSelection  (Selection::Marker const & selection)
{
	_countAtomicAction++;
	_recorder.Write (selection.Start ());
	_recorder.Write (selection.End ());
	_recorder.WriteChar (SimpleAction::newSelection);
	dbg << SimpleAction::newSelection;
}

void EditLog::MakeInfo (char const * buf, int lenBuf)
{
	if (!_curActionInfo.empty ())
		return;
	lenBuf = std::min<int> (lenBuf, MAX_INFO_LEN);
	_curActionInfo.append (buf, lenBuf);
}

void EditLog::PositionBegin (DocPosition const & docPos)
{
	_countAtomicAction++;	
	_recorder.Write (docPos);
	_recorder.WriteChar (SimpleAction::positionBegin);
	dbg << SimpleAction::positionBegin;
}

void EditLog::PositionEnd (DocPosition const & docPos)
{
	_posActionEnd = docPos;
}

void RedoLog::BeginAction (Action::Bits action, DocPosition const & docPos, bool isSelection)
{
	PositionBegin (docPos);
	_curAction = action;	
}

void RedoLog::EndAction (DocPosition const & docPos)
{
	PositionEnd (docPos);
	_stateSink->BackwardAction ();
	if (_endLastAction == 0) // if first action on the log
		_stateSink->OnChangeState ();
	Flush ();
}

void RedoLog::InsertChar (DocPosition const & docPos, char c, EditStyle style)
{
	int lenSegment = 1;
	Selection::ParaSegment paraSeg (docPos, lenSegment);
	char const * buf = &c;
	Insert (paraSeg, buf, style);
}
	
void RedoLog::Clear () throw ()
{	
	if (_canClear)
		EditLog::Clear ();
}

// continueTyping is treated differently, because we try to accumulate
// multiple inserts into a single action. As long as the user continues
// typing in one place, we keep extending this action.
void UndoLog::BeginAction (Action::Bits action, DocPosition const & docPos, bool isSelection)
{
	if (action == Action::continueTyping)
	{  // Keep extending current typing action if possible
		if (_curAction == Action::typing && !isSelection && CanAppendTyping (docPos))
			return;
		else
			action = Action::typing;
	}
	Flush ();
	PositionBegin (docPos);
	_curAction = action;	
}

void UndoLog::EndAction (DocPosition const & docPos)
{
	// even if the action is empty , docPosBegin is logged	
	Assert (_countAtomicAction > 0);
	if (!_isEmptyAction ) // if no empty action
	{
		PositionEnd (docPos);
		_redoLog.Clear ();

		// notification ForwardAction 
		if (_curAction != Action::typing) // Action completed: notify.
			_stateSink->ForwardAction ();
		else if (_lenTyping == 1) // Action typing. Notify only on first typing.
			_stateSink->ForwardAction ();
		else if (_lenTyping == 0) // Action typing, special case. This typing action comes from redo log: notify.
			_stateSink->ForwardAction ();
		
		if (_endLastAction == 0) // if first action in the log
			_stateSink->OnChangeState ();
		if (_curAction != Action::typing) // we must wait for the next action
			Flush ();
	}
	else 
	{
		// action is empty, we must remove start position
		_recorder.EraseAt (_endLastAction);
		_countAtomicAction = 0;
		_curAction = Action::none;
	}	
}

void UndoLog::Flush ()
{
	if (_countAtomicAction == 0)
		return;
	
	Assert (_curAction != Action::none);
	// Finish atomic insert for incremental typing
    // Notice: redo never uses incremental typing
	if (_curAction == Action::typing  && _lenTyping > 0)
	{
		_recorder.WriteLong (_lastTypePos.ParaOffset () + _lenTyping);
		_recorder.WriteChar (SimpleAction::insert);
		dbg << SimpleAction::insert;
		_lenTyping = 0;
	}
	EditLog::Flush ();
}

void UndoLog::InsertChar (DocPosition const & docPos, char c, EditStyle style)
{
	Assert (CanAppendTyping (docPos));
	AppendTyping (docPos, c, style);
}

bool UndoLog::CanAppendTyping (DocPosition const & docPos)
{
	return (_lenTyping == 0    // begin typing
		   || (docPos.ParaNo () == _lastTypePos.ParaNo () && docPos.ParaOffset () == _lastTypePos.ParaOffset () + _lenTyping));
}

void UndoLog::AppendTyping (DocPosition const & docPos, char c, EditStyle style)
{
	if (_curActionInfo.size () < unsigned int (MAX_INFO_LEN))
		_curActionInfo += c;	

	if (_lenTyping == 0) // if new typing
	{
		_lastTypePos = docPos;
		_recorder.WriteStyle (style);
		_recorder.Write (docPos);
		_countAtomicAction++;
		_isEmptyAction = false;
	}
	_lenTyping++;
}

void UndoLog::Clear () throw ()
{
	_lenTyping = 0;
	_lastTypePos = DocPosition ();
	EditLog::Clear ();
}