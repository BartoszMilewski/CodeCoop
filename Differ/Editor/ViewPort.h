#if !defined (VIEWPORT_H)
#define VIEWPORT_H
//
// (c) Reliable Software 1997 -- 2002
//
#include "Mapper.h"
#include "Document.h"

// Maps document (line, col) position into 
// window (line, col) position and vice versa
class EditParagraph;
class LineNotificationSink;

class ParaNotificationSink
{
public:
	virtual ~ParaNotificationSink () {}

	virtual void OnParaLenChange (DocPosition const & pos, EditParagraph const * para, int deltaLen) = 0;
	virtual void OnStyleChanged (int paraNo) = 0;
	virtual void OnMergeParagraphs (int startDocPara, EditParagraph const * newPara) = 0;
	virtual void OnSplitParagraph (int paraNo, EditParagraph const * firstPara, 
		                           EditParagraph const * secondPara) = 0;
	virtual void OnDelete (int startParaNo, int endParaNo, EditParagraph const * newPara) = 0;
	virtual void InsertParagraph (int paraNo, EditParagraph const * newPara) = 0;
	virtual void InsertParagraphs (int paraNo, std::vector<EditParagraph const *> newParagraphs) = 0;
};

class SelectionNotificationSink
{
public:
	virtual ~SelectionNotificationSink () {}

	virtual void OnChangeSelection (int startParaNo, int startOffset, int lastParaNo, int lastOffset) = 0;
};

class ViewPort : public SelectionNotificationSink, public ParaNotificationSink
{
public:
    ViewPort (LineNotificationSink * sink = 0, int tabOff = 0);
	// Most virtual functions implemented below are dummy.
	// They couldn't be implmented as pure virtual, becouse object ViewPort is to be 
	// temporary created when initiating program.
	virtual bool ColIsOutside (int visCol) const { return false; }
	virtual int  PageUp () { return 0; }
	virtual int PageDown (int docLen) { return 0; }
	virtual int Up () { return 0; }
	virtual int Down (int docLen) { return 0; }
	virtual int ParaToWinClip (int para, int offset) { return 0; } 
	virtual int GetVerScrollPos () const { return 0; }
	virtual void SetSize (int visCols, int visLines, const Document & doc) {}
	virtual int ParaToWin (int para, int paraOffset = 0) { return 0; }
	virtual int ParaToLine (int para, int offset) { return 0; }
	virtual int ColDocToWin (int col, EditParagraph const * para) const { return 0; }
	virtual int GetMaxParagraphLen () const { return 0; }
	virtual int LineWinToPara (int visLine, int & offset) const { return 0; }
    virtual int	LineWinToPara (int line) const { return 0;}
	virtual int GetDocLines (const Document & doc) const { return 0; }
	virtual int SetScrollPosition (EditParagraph const * para, int line) { return 0; }
	// return true if line exist
	virtual bool GetLineBoundaries (int visLine, EditParagraph const * para, 
		                            int & paraOffsetBegin, int & paraOffsetEnd) const {return false;}
	virtual void GetLine (int visLine, int & paraOffsetBegin, int & paraOffsetEnd) const {}
    virtual void NewDoc (const Document & doc) {}
	virtual int LineToPara (int line) const { return 0; }
	virtual int ColWinToDocAdjust (int & visCol, int & visLine, EditParagraph const * para)  const
	{
		return 0;
	}
	virtual int GetDocAnchorCol (EditParagraph const * curPara, int line = 0) const { return 0; }
	virtual int SetDocCol (int docCol) { return 0; }
	virtual int VisibleParaOffset (EditParagraph const * para)  const { return 0; }
	virtual bool UpdateMaxParagraphLen (EditParagraph const * para) {return true;}
	virtual void GoToDocBegin ();
	// Returns true if adjustment successful (current position visible)
	virtual bool AdjustDocPos (EditParagraph const * currPara) { return false;}
	virtual int Right (int & currParaNo, EditParagraph const * currPara, int docLen) { return 0;}
	virtual int Left (int & currParaNo, EditParagraph const * currPara, EditParagraph const * prevPara) { return 0;}
	int GetColOffset () const { return _scrollPos._horScrollPos;}
	// Lines
    int Lines () const { return _visLines; }
    bool LineIsOutside (int line) const;
	void SetDocParaOffset (int paraOfset)
	{
		_scrollPos._docParaOffset = paraOfset;
	}
	int GetOffset () const
	{
		return _scrollPos._docParaOffset;
	}	
	int GetParagraphNo () const { return _scrollPos._docPara; }

	// Columns
    int GetVisCols () const { return _visCols; }
    int ColDocToWinClip (int col, EditParagraph const * para) const;	
    void SetAnchorCol (int col, EditParagraph const * curParagraph);
    int GetVisibleAnchorCol () const;
	int GetExpandedDocCol (int col, EditParagraph const * para) const;
	void SetTabSize (int tabLenChar) { _tabber.ChangeTabLen (tabLenChar); }
    // Navigation
    void SetDocPos (int col, int line);
	void SetDocPosition  (DocPosition const & docPos);
    int GetCurPara () const { return _docPara; }
    int GetCurDocCol () const { return _docCol; } 
	DocPosition  GetDocPosition ( ) { return DocPosition (_docPara, _docCol);}
    int LeftWord (int & docLine, EditParagraph const * curPara, int prevParaLen);
    int RightWord (int & docLine, EditParagraph const * curPara, int docLen);
	void SelectWord (int & wordBegin, int & wordEnd, EditParagraph const * curPara);
    void DocBegin ();
    void LineBegin ();
    void DocEnd (int lastLineLen, int docLen);
    void LineEnd (int lineLen);
	void ForceRight ()
	{
		_docCol++;
		_anchorCol = _docCol;
	}
	void ForceLeft ()
	{
		_docCol--;
		_anchorCol = _docCol;
	}
	// ParaNotyficationSink
    virtual void OnParaLenChange (DocPosition const & pos, EditParagraph const * para, int deltaLen) {}
    virtual void OnStyleChanged (int paraNo) {}
	virtual void OnMergeParagraphs (int startDocPara, EditParagraph const * newPara) {}
	virtual void OnSplitParagraph (int paraNo,EditParagraph const * firstPara, 
		                           EditParagraph const * secondPara) {}
	virtual void OnDelete (int startParaNo, int endParaNo, EditParagraph const * newPara) {}
	virtual void InsertParagraph (int paraNo, EditParagraph const * newPara) {}
	virtual void InsertParagraphs (int paraNo, std::vector<EditParagraph const *> newParagraphs) {}
	// SelectionNotificationSink
	virtual void OnChangeSelection (int startParaNo, int startOffset, int lastParaNo, int lastOffset) {}
protected:
    int PrevWord (char const * line);
    int NextWord (char const * line, int lineLen);
	void AdjustDocPos (const Document & doc);
	

protected:
	class ScrollPos
	{
	public:
		ScrollPos ()
			: _docPara (0), _docParaOffset (0), _horScrollPos (0)
		{}
	public:
		int _docPara;
		int _docParaOffset;
		int _horScrollPos;
	};

protected:
	Tabber	_tabber;
	// position of upper left corner of viewport in document
	ScrollPos	_scrollPos;
	// viewport size
    int     _visCols;
    int     _visLines;
	// caret position
    int     _docCol;
    int     _docPara;
	// keep track of starting column during vertical cursor movements
    int     _anchorCol;
	// needed for horizontal scollbar display
	int		_maxLineLen;
	LineNotificationSink * _lineSink;
};

class ScrollViewPort : public ViewPort
{
public :
    ScrollViewPort (const ViewPort * otherPort, const Document & doc);
	// lines 
	int ParaToWinClip (int para, int offset)  
    { 
        int visLn = para - _scrollPos._docPara; 
        if (visLn < 0 || visLn >= _visLines)
            return -1;
        return visLn;
    }
	int ParaToWin (int para, int paraOffset = 0)
	{ 
        return para - _scrollPos._docPara; 
    }
	int ParaToLine (int para, int offset) {return para; }
	int LineWinToPara (int visLine, int & offset) const;
	int	LineWinToPara (int line) const { return line + _scrollPos._docPara;}
	int LineToPara (int line) const {return line;}
	bool AdjustDocPos (EditParagraph const * currPara);
	// true if new _maxParagraphLen
	bool UpdateMaxParagraphLen (EditParagraph const * para);
	int GetDocLines (const Document & doc) const;
	void GetLine (int visLine, int & paraOffsetBegin, int & paraOffsetEnd) const;
	// return true if line exist
	bool GetLineBoundaries (int visLine, EditParagraph const * para, 
		                    int & paraOffsetBegin, int & paraOffsetEnd) const;
	// columns
    bool ColIsOutside (int visCol) const;
	int ColDocToWin (int col, EditParagraph const * para) const;
	int ColWinToDocAdjust (int & visCol, int & visLine, EditParagraph const * para)  const;
	int GetDocAnchorCol (EditParagraph const * curPara, int line = 0) const;
	int SetDocCol (int docCol);
	int VisibleParaOffset (EditParagraph const * para)  const;
	int GetMaxParagraphLen () const { return _maxLineLen; }
	// navigation
    int  PageUp ();
    int PageDown (int docLen);
    int Up ();
    int Down (int docLen);    
    int GetVerScrollPos () const { return _scrollPos._docPara; }    	
	int SetScrollPosition (EditParagraph const * para, int line);
	int Right (int & currParaNo, EditParagraph const * currPara, int docLen);
	int Left (int & currParaNo, EditParagraph const * currPara, EditParagraph const * prevPara);

	void SetSize (int visCols, int visLines, const Document & doc)
	{
		_visCols = visCols;
		_visLines = visLines;
	}
    void NewDoc (const Document & doc);
	// ParaNotyficationSink
    void OnParaLenChange (DocPosition const & pos, EditParagraph const * para, int deltaLen);
	void OnStyleChanged (int paraNo);
    void OnMergeParagraphs (int startDocPara, EditParagraph const * newPara);
	void OnSplitParagraph (int paraNo, EditParagraph const * firstPara, 
		                   EditParagraph const * secondPara);
	void OnDelete (int startParaNo, int endParaNo, EditParagraph const * newPara);
	void InsertParagraph (int paraNo, EditParagraph const * newPara);
	void InsertParagraphs (int paraNo, std::vector<EditParagraph const *> newParagraphs);
	void OnChangeSelection (int startParaNo, int startOffset, int lastParaNo, int lastOffset);
};

// Revisit: not fully implemented
class BreakViewPort : public ViewPort
{
public:
	BreakViewPort (const ViewPort * otherPort, const Document & doc);
	// Lines	
	int LineToPara (int line) const;
	int	LineWinToPara (int line, int & offset) const;
	int	LineWinToPara (int line) const;
	int ParaToWin (int para, int paraOffset = 0);
    void GetLine (int visLine, int & paraOffsetBegin, int & paraOffsetEnd) const;
	// return true if line exist
	bool GetLineBoundaries (int visLine, EditParagraph const * para, 
		                    int & paraOffsetBegin, int & paraOffsetEnd) const;
	int ParaToLine (int para ,int offset);
	int ParaToWinClip (int para, int offset);
	// columns
	int ColDocToWin (int col, EditParagraph const * para) const;
	int SetDocCol (int docCol);
	bool ColIsOutside (int visCol) const { return false; }
	int GetDocAnchorCol (EditParagraph const * curPara, int line = 0) const;
	int ColWinToDocAdjust (int & visCol, int & visLine, EditParagraph const * para)  const;
	int GetMaxParagraphLen () const { return _visCols - 1; }
	bool AdjustDocPos (EditParagraph const * currPara);
	// if return true then scrollbars are refreching
	bool UpdateMaxParagraphLen (EditParagraph const * para) { return true ; }	    	
    // navigation    	
	int Up ();
	int Down (int docLen);
	int PageDown (int docLen);
	int PageUp ();
	int GetDocLines (const Document & doc) const;
	int VisibleParaOffset (EditParagraph const * para)  const { return 0; }
	int SetScrollPosition (EditParagraph const * para, int line);	
	int GetVerScrollPos () const { return _lineScrollPos; }
	int Right (int & currParaNo, EditParagraph const * currPara, int docLen);
	int Left (int & currParaNo, EditParagraph const * currPara, EditParagraph const * prevPara);
	// formating
	void SetSize (int visCols, int visLines, const Document & doc);
	void NewDoc (const Document & doc);
	void GoToDocBegin ();

	// ParaNotyficationSink
	void OnParaLenChange (DocPosition const & pos, EditParagraph const * para, int deltaLen);
    void OnStyleChanged (int paraNo);
	void OnMergeParagraphs (int startDocPara, EditParagraph const * newPara);
	void OnSplitParagraph (int paraNo, EditParagraph const * firstPara, 
		                   EditParagraph const * secondPara);
	void OnDelete (int startParaNo, int endParaNo, EditParagraph const * newPara);
	void InsertParagraph (int paraNo, EditParagraph const * newPara);
	void InsertParagraphs (int paraNo, std::vector<EditParagraph const *> newParagraphs);
	// SelectionNotificationSink
	void OnChangeSelection (int startParaNo, int startOffset, int lastParaNo, int lastOffset);
	
private:
	int LastLineInParagraph (int paraNo);
	void FormatDoc (const Document & doc);
	void RefreshLines (int startPaintLine, int endPaintLine, int deltaLines);
	 // true = more lines
	bool GetLine (EditParagraph const * para, int paraOffsetBegin, int & paraOffsetEnd) const;

	
	class VisLine
	{
	public :
		friend class BreakViewPort;
		VisLine (int para = 0, int paraOffset = 0)
			: _para(para), _paraOffset (paraOffset)
		{}
		bool operator < (const VisLine & line) const
		{
			if (_para != line._para)
				return _para < line._para;
			return _paraOffset  < line._paraOffset;
		}
	private :
		int _para;
		int _paraOffset;
	};
	int _lineScrollPos;  // top visible line
	std::vector<VisLine> _lines; // all doucument's lines
};

#endif
