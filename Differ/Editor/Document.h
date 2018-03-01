#if !defined DOCUMENT_H
#define DOCUMENT_H

#include "Lines.h"
#include "DocPosition.h"
#include "Search.h"

class Tabber;
class ClipboardText;
class ParaNotificationSink;
class EditLog;


//
// Future: retrieve data from "current document"
//
class Document
{
	friend class FindPredicate;
public:
    Document (EditLog * editLog);
    ~Document ();
    void Init (std::unique_ptr<EditBuf> lineBuf);
    int GetParagraphCount () const 
    { 
        return _paragraphs.get () ? _paragraphs->GetCount (): 0; 
    }
	int GetDocEndParaPos () const;
    EditParagraph * const GetParagraph (int i) const 
	{
		if (i >= GetParagraphCount ())
			return 0;
		return _paragraphs->GetParagraph (i); 
	}
    EditParagraph * GetParagraph (int i) 
	{
		if (i >= GetParagraphCount ())
			return 0;
		return _paragraphs->GetParagraph (i); 
	}
    EditParagraph * GetParagraphAlways (int i);
    int GetParagraphLen (int i) const;
    int GetMaxParagraphLen () const 
    { 
        return _maxParagraphLen; 
    }
    int NextChange (int startPara) const;
    int PrevChange (int startPara) const;

	void SplitParagraph (DocPosition const & docPos, EditStyle style1 = UserInsertStyle (), EditStyle style2 = UserInsertStyle ());
	void AppendParagraph (int paraNo);	 
	void InsertParagraphs (DocPosition const & docPos, EditBuf const & newPara, int targetParaNo);
	void DeleteChar (DocPosition const & docPos);
	void InsertChar (DocPosition const & docPos, int c, EditStyle style = UserInsertStyle ());	
	bool Find ( SearchRequest const * data, Selection::ParaSegment & inOut);
	void SetParaNotificationSink (ParaNotificationSink * sink) { _paraSink = sink; }
	void MergeParagraphs (int startParaNo, EditStyle style = UserInsertStyle ());
	void Delete (Selection::Marker const & selection);
	void Replace (Selection::ParaSegment const & segment, const std::string & text);
	int  ReplaceAll (ReplaceRequest const * data, DocPosition & curPos);
	void SetEditLog (EditLog * editLog) {_editLog = editLog; }
	void InsertBuf (DocPosition const & docPos, char const * buf, int lenBuf, EditStyle style = UserInsertStyle ());
	void InsertParagraph (int paraNo, char const * buf, int lenBuf, EditStyle style = UserInsertStyle ());
	void CutFromParagraph (Selection::ParaSegment const & segment, EditStyle style = UserInsertStyle ());
	void DeleteParagraph (int paraNo);
	void AddTabs (int firstParaNo, int lastParaNo);
	int DeleteTabOrSpaces (int paraNo, int tabSize); // return deltaLen
private:
	void DeleteParagraphs (std::vector<int>::iterator begin, std::vector<int>::iterator end) 
	{ 
		_paragraphs->Delete (begin, end); 
	}
	void InsertCompleteParagraphs (int paraNo, EditBuf const & newPara, int targetParaNo);
	void InsertParagraph (int paraNo, std::unique_ptr<EditParagraph> & newPara)
	{
		_paragraphs->InsertParagraph (paraNo, newPara);
	}
	EditStyle GetStyleParaNo (int paraNo) const;
private:
	std::unique_ptr<EditBuf>  _paragraphs;
	int						_maxParagraphLen;
	ParaNotificationSink *	_paraSink;
	EditLog		*			_editLog;

private:
	class Finder
	{
	public:
		Finder (SearchRequest const * req);
		bool FindIn (char const * buf, int lenBuf, int startOffset);
		bool IsDirectionForward () const { return _serchReq.IsDirectionForward (); }
		int GetResultOffset () const { return _startOffset; }
		int GetResultLen () const {return _findWord.size (); }
	private:
		bool MatchForward (char const * buf, int lenBuf) ;
		bool MatchBackward (char const * buf, int lenBuf) ;
		bool IsWholeWord (char const * buf, int lenBuf) const;
		bool MatchInsensitive (char const * buf, int lenBuf) const;
		bool MatchSensitive (char const * buf, int lenBuf)const;

		SearchRequest _serchReq;
		std::string   _findWord; // may get modified (uppercased)
		int           _startOffset;
	};

	class FindPredicate
	{
	public:
		FindPredicate (Finder & finder)
			: _finder (finder)
		{}
		bool operator () (const EditParagraph * para);
	private:
		Finder & _finder;
	};
};

#endif
