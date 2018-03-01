#if !defined (EDACT_H)
#define EDACT_H
//
// Reliable Software (c) 1998 -- 2002
//
#include "DocPosition.h"
#include "EditStyle.h"

class EditParagraph;
class EditStateNotificationSink;


// Logging is part of the undo mechanism

// action := positionBegin + deselect (optional) + one or more simple actions + positionEnd  + 
// count simple actions + info + action name
// actions names : newLine, insertChar, del, backSpace, paste, cut, replace, replaceAll (encoded in a single byte)
// Action (and its name and info) corresponds to the user's edit action and is undone in one step
// Every action can to be split into one or more simple actions, 
//     for example: insertChar can be really deSelect + cutFromParagraph + pasteWithoutEndl
//                  its undo consists of deleting a char, pasting deleted text and restoring selection
// simple action := one or more int parametrs + (optional) buf + lenbuf + simple action name
// simple action names : cutFromParagraph, cutParagraphEnd, cutCompleteParagraph, pasteWithoutEndl,
//                      insertParagraph, pasteWithEndl, splitParagraph, mergeParagraphs, appendEmptyParagraph,
//                      deleteChar,
// deSelect := int parametrs + action name.
// positionBegin := int parametrs + action name
// positionEnd := int parametrs + action name
// info := buf + lenBuf. It is private (editLog's) record -  for  display purposes only
//			lenBuf < 256 and is coded as char.
//          Info is optional - the principle : bool EditLog::ActionHasInfo (Action  action)

class EditLog
{
protected:
	typedef std::deque<char>::reverse_iterator SourceIter;
public:
	class Input
	{
	public:
		Input (EditLog & log)
			:_sourceIter (log.SourceBegin ()), _sourceEnd (log.SourceEnd ())
		{}
		char GetChar ();
		long GetLong ();
		EditStyle GetStyle ();
		void GetString (std::string & str);
		void GetSmallString (std::string & str);
		DocPosition GetDocPosition ();
		EditLog::SourceIter  GetCurrentLogPosition () { return _sourceIter; }
	private :
		EditLog::SourceIter  _sourceIter;
		EditLog::SourceIter  _sourceEnd;
	};

	struct Action    // bellow actions corresponds to the user's edit action 
	{
		enum Bits
		{
			typing,          
			cut,
			paste,
			del,
			replaceAll,
			backSpace,
			newLine,
			replace,
			autoIndent,
			continueTyping, // continueTyping is loging as typing 
			multiParaTabAdd,
			multiParaTabDel,
			none        // no action
		};
	};
	
	struct SimpleAction
	{
		enum Bits
		{
			cutFromParagraph,	// cut within a single paragraph not including endline
			cutCompleteParagraph,		
			insertCompleteParagraph,
			insert,				// paste within single paragraph without adding endline
			splitParagraph,
			mergeParagraphs,	// remove dividing endline
			appendEmptyParagraph,		
			deleteChar,			
			oldSelection,
			newSelection,
			positionBegin,
			positionEnd,
			infoBuf				// marks additional info for display
		};
	};

public:
	EditLog (EditStateNotificationSink * stateSink);
	virtual ~EditLog () {}

	virtual void BeginAction (Action::Bits action, DocPosition const & docPos, bool isSelection) = 0; // begin of logging  user's edit actions	
	virtual void EndAction (DocPosition const & docPos) throw () = 0 ;
	virtual void Flush ();	
	bool Empty () { return _recorder.Empty (); }
	void Erase (SourceIter iter);
	virtual void Clear () throw ();
	
	// logging simple actions :
	void CutFromParagraph (Selection::ParaSegment const & paraSeg, EditParagraph const * para, EditStyle style);
	void CutCompleteParagraph (int paraNo, EditParagraph const * para, EditStyle style);
	void SplitParagraph (DocPosition const & docPos, EditStyle style);
	void MergeParagraphs (DocPosition const & docPos, EditStyle style1, EditStyle style2);
	void AppendEmptyParagraph (int paraNo);
	void DeleteChar (DocPosition const & docPos, char c, EditStyle style);	
	void Insert (Selection::ParaSegment const & paraSeg, char const * buf, EditStyle style);
	void InsertCompleteParagraph (int paraNo, EditParagraph const * para);
	void Replace (DocPosition const & docPos, std::string const & oldStr, std::string const & newStr, EditStyle style);
	void OldSelection (Selection::Marker const & selection);
	void NewSelection (Selection::Marker const & selection);
	virtual void InsertChar (DocPosition const & docPos, char c, EditStyle style) = 0;

	// for execute undo	
	SourceIter SourceBegin () {return _recorder.SourceBegin (); }
	SourceIter SourceEnd () {return _recorder.SourceEnd (); }
	
protected:
	class Recorder
	{
	public:
		void WriteLong (long i);
		void WriteStyle (EditStyle const & style);
		void WriteChar (char c) {_buf.push_back (c); }
		void WriteText (int lenBuf, char const *  text);
		void Write (DocPosition const & docPos);
		void Write (Selection::ParaSegment const & paraSeg);
		void WriteSmallText (int lenBuf, char const *  text); // lenbuf < 256
		bool Empty () { return _buf.empty ();}
		int Size () { return _buf.size ();}
		void EraseAt (int n) {_buf.erase (_buf.begin () + n, _buf.end ()); } 
		void Erase (SourceIter iter);
		void Clear () {_buf.clear (); } 
		SourceIter SourceBegin () {return _buf.rbegin (); }
		SourceIter SourceEnd () {return _buf.rend (); }
	private:
		std::deque<char>	_buf;
	};
	enum { MAX_INFO_LEN = 5}; // in this implementation MAX_INFO_LEN must be less than 256

	void MakeInfo (char const * buf, int lenBuf);
	void PositionBegin (DocPosition const & docPos);
	void PositionEnd (DocPosition const & docPos);
	
	Recorder		_recorder;
	// state of current Action
	Action::Bits	_curAction;
	std::string		_curActionInfo;			// For display purposes
	long			_countAtomicAction;
	DocPosition     _posActionEnd;
	int				_endLastAction;
	EditStateNotificationSink * _stateSink;
	bool            _isEmptyAction;
};

class RedoLog : public EditLog
{
public:
	RedoLog (EditStateNotificationSink * stateSink)
		:EditLog (stateSink), _canClear (true)
	{}
	void BeginAction (Action::Bits  action, DocPosition const & docPos, bool isSelection);
	void EndAction (DocPosition const & docPos) throw ();
	void InsertChar (DocPosition const & docPos, char c, EditStyle style);	
	void Clear ();
	void SetCanClear (bool canClear) {_canClear = canClear; } 
private :
	bool	_canClear;
};

class UndoLog : public EditLog
{
public:
	UndoLog (EditStateNotificationSink * stateSink, RedoLog & redoLog)
		:EditLog (stateSink), _redoLog (redoLog), _lenTyping (0)
	{}
	void BeginAction (Action::Bits action, DocPosition const & docPos, bool isSelection);
	void EndAction (DocPosition const & docPos) throw ();
	void Flush ();
	void InsertChar (DocPosition const & docPos, char c, EditStyle style);
	void Clear () throw ();
	
private:
	bool CanAppendTyping (DocPosition const & docPos);
	void AppendTyping (DocPosition const & docPos, char c, EditStyle style);
private :
	long			_lenTyping; // Cumulative length of typing action
	DocPosition     _lastTypePos; // Last typing position
	RedoLog &		_redoLog;
};

#endif
