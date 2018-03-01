#if !defined (PARSER_H)
#define PARSER_H

//
// Reliable Software (c) 2001
//

#include "EditLog.h"
#include "Select.h"

class UndoSink;

class UndoParser
{
public:
	UndoParser (EditLog & editLog, UndoSink & undoSink);
	void Execute ();
	Selection::Marker & GetSelection ();
	DocPosition & GetPosition ();

private :
	void Action ();
	void ReadInfo ();
	void SimpleAction ();
	void CutFromParagraph ();
	void CutCompleteParagraph ();
	void InsertCompleteParagraph ();
	void Insert ();
	void SplitParagraph ();
	void MergeParagraphs ();
	void AppendEmptyParagraph ();
	void DeleteChar ();
	void OldSelection ();
	void NewSelection  ();
	void PositionBegin ();
	void PositionEnd ();
private:
	EditLog			& _editLog;
	EditLog::Input	  _input;
	UndoSink		& _undoSink;
	std::string      _info;
	std::string      _simpleActionBuf;
	EditLog::Action::Bits  _actionName;

	//selection for restore 
	Selection::Marker _selection;
	//position for restore
	DocPosition     _docPosition;
};

#endif
