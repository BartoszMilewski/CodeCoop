#if !defined (UNDOSINK_H)
#define UNDOSINK_H
//-----------------------------------
// Reliable Software (c) 2001 -- 2002
//-----------------------------------

#include "DocPosition.h"
#include "EditLog.h"
#include "EditStyle.h"

class Document;

class UndoSink 
{
public :
	UndoSink (Document & doc, EditLog & editLog)
		: _doc (doc),
		  _editLog (editLog)
	{}

	void BeginAction (EditLog::Action::Bits  actionName, DocPosition const & docPos);
	void EndAction (DocPosition const & docPos);
	void CutFromParagraph (DocPosition const & docPos, std::string const & buf, EditStyle style);
	void CutCompleteParagraph (int paraNo, std::string const & buf, EditStyle style);
	void InsertCompleteParagraph (int paraNo);
	void Insert (Selection::ParaSegment const & seg, EditStyle style);
	void SplitParagraph (DocPosition const & docPos, EditStyle style);
	void MergeParagraphs (DocPosition const & docPos, EditStyle style1, EditStyle style2);
	void AppendEmptyParagraph (int paraNo);
	void DeleteChar (DocPosition const & docPos, char c, EditStyle style);
	void OldSelection  (Selection::Marker const & selection);
	void NewSelection  (Selection::Marker const & selection);

private:
	Document &	_doc;
	EditLog	&	_editLog;
};

#endif
