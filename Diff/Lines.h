#if !defined LINES_H
#define LINES_H
//-----------------------------------------
// (c) Reliable Software 1997 -- 2002
//-----------------------------------------

#include "ListingWin.h"

#include <auto_vector.h>
#include <Dbg/Assert.h>

class LineBuf;

//
// Editable lines
//

class EditParagraph
{
public:
    EditParagraph (char const * buf, int len, EditStyle style, int paraNo)
		:	_len (len),
			_size (len),
			_style (style),
			_paraNo (paraNo),
			_buf (new char [len])
    {
        memcpy (_buf, buf, len);
    }
    EditParagraph (char const * buf, int len, int paraNo)
		:	_len (len),
			_size (len),
			_paraNo (paraNo),
			_buf (new char [len])
    {
        memcpy (_buf, buf, len);
    }
    ~EditParagraph ()
    {
        delete []_buf;
    }
    char * Buf () { return _buf; }
	int operator [] (int i) const
	{
		Assert (i >= 0 && i < _len);
		return _buf [i];
	}
    char const * Buf () const { return _buf; }
	char const * BufAt (int offset) const  { return &_buf [offset]; }
    int Len () const { return _len; }
	void SetLen (int len) { _len = len; }
	void SetStyle (EditStyle style) { _style = style; }
	void DeleteChar (int pos);
	void InsertChar (int pos, int c);
	int GetParagraphNo () const { return _paraNo; }
    EditStyle GetStyle () const { return _style; }
	bool IsChanged ()  const { return _style.IsChanged (); }
	bool IsSpace (int offset) const;
	void Replace (int startOffset, int endOffset, const std::string & text);

private:	
	bool IsWordChar (char c);
	void ReserveMore (int len);

private:
	char *		_buf;
    int			_len;
	int			_size;
    EditStyle	_style;
	int			_paraNo;
};

class EditBuf : public ListingWindow
{
public:
    EditBuf ()
		: _lastLF (false)
	{}

	void Init (LineBuf const & lineBuf);
	void InsertParagraph (int paraNo, std::unique_ptr<EditParagraph> & newLine);
	void Delete (std::vector<int>::iterator begin, std::vector<int>::iterator end);
	void Delete (int firstParaNo, int lastParaNo);
	void Delete (int paraNo);
	typedef auto_vector<EditParagraph>::iterator iterator;
	typedef auto_vector<EditParagraph>::reverse_iterator reverse_iterator;
	iterator begin () { return _paragraphs.begin ();}
	iterator end () { return _paragraphs.end ();}
	reverse_iterator rbegin () { return _paragraphs.rbegin ();}
	reverse_iterator rend () { return _paragraphs.rend ();}
	iterator ToIter (int idx) { return _paragraphs.ToIter (idx); }
	reverse_iterator ToRIter (int idx) { return _paragraphs.ToRIter (idx); }
	int ToIndex (iterator & it) { return _paragraphs.ToIndex (it); }
	int ToIndex (reverse_iterator & rit) { return _paragraphs.ToIndex (rit); } 
    EditParagraph * GetParagraph (int i) { return _paragraphs [i]; }
    EditParagraph const * GetParagraph (int i) const { return _paragraphs [i]; }
    int GetCount () const { return _paragraphs.size (); }
	bool IsNewParagraphAtTextEnd () const { return _lastLF; }

	// Listing window interface
	void PutLine (char const * buf, unsigned int len, EditStyle act, int paraNo);
    void PutLine (char const * buf, unsigned int len, int paraNo);

protected:
    auto_vector<EditParagraph>	_paragraphs;
	bool						_lastLF;
};

// Accepts only lines that appear in the target
// Ignores deleted and cut
class EditBufTarget : public EditBuf
{
public:
	// Listing window interface
	void PutLine (char const * buf, unsigned int len, EditStyle act, int paraNo);
};

#endif
