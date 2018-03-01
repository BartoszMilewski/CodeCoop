//-----------------------------------------
// (c) Reliable Software 1997 -- 2002
//-----------------------------------------

#include "precompiled.h"
#include "Lines.h"
#include "LineCounter.h"
#include "Cluster.h"
#include "LineBuf.h"

#include <StringOp.h>

void EditParagraph::DeleteChar (int pos)
{
	Assert (_len > 0);
	Assert (pos < _len);
	// ripple copy
	for (int i = pos; i < _len - 1; i++)
		_buf [i] = _buf [i + 1];
	_len--;
}

void EditParagraph::InsertChar (int pos, int c)
{
	Assert (pos <= _len);
	Assert (_len <= _size);
	if (_len + 1 > _size)
		ReserveMore (1);	
	// ripple copy
	for (int i = _len - 1; i >= pos; i--)
		_buf [i + 1] = _buf [i];
	_buf [pos] = c;
	_len++;
}

void EditParagraph::Replace (int startOffset, int endOffset, const std::string & text)
{
	Assert (startOffset >= 0 && startOffset < _len);
	Assert (endOffset > 0 && endOffset <= _len);
	int deltaLen = text.size () - (endOffset - startOffset);
	int newLen = _len + deltaLen;
	if (newLen > _size)
		ReserveMore (newLen);

	if (deltaLen < 0)
		std::copy (_buf + endOffset, _buf + _len, _buf + endOffset + deltaLen);
	else
		std::copy_backward (_buf + endOffset, _buf + _len, _buf + newLen);

	std::copy (text.begin (), text.end (), _buf + startOffset);
	_len = newLen;
}

void EditParagraph::ReserveMore (int len)
{
	Assert (len >= 0);
	int newSize = (_size >= 4)? _size * 2 + len: 4 + len;
	char * bufNew = new char [newSize];
	std::copy (_buf, _buf + _len, bufNew);
	delete []_buf;
	_buf = bufNew;
	_size = newSize;
}

bool EditParagraph::IsSpace (int offset) const
{
	return ::IsSpace ( _buf [ offset]);
}

void EditBuf::Init (LineBuf const & lineBuf)
{
    for (unsigned int i = 0; i < lineBuf.Count (); i++)
    {
        Line const * line = lineBuf.GetLine (i);
        char const * buf = line->Buf ();
        int len = line->Len ();
        while (len > 0 && IsEndOfLine (buf [len-1]))
            len--;
        std::unique_ptr<EditParagraph> newPara (new EditParagraph (buf, len, i));
        _paragraphs.push_back (std::move(newPara));
    }
	Line const * lastLine = lineBuf.GetLine (lineBuf.Count () - 1);
	char const * buf = lastLine->Buf ();
	int lastCharIndex = lastLine->Len () - 1;
	_lastLF = buf [lastCharIndex] == '\n';
}

void EditBuf::PutLine (char const * buf, unsigned int len, int paraNo)
{
	EditStyle editStyle;
	PutLine (buf, len, editStyle, paraNo);
}

void EditBuf::PutLine (char const * buf, unsigned int len, EditStyle act, int paraNo)
{
	if (len != 0)
	{
		_lastLF = IsEndOfLine (buf [len - 1]);
		while (len != 0 && IsEndOfLine (buf [len - 1]))
			len--;
	}
	else
	{
		_lastLF = false;
	}
    std::unique_ptr<EditParagraph> newPara (new  EditParagraph (buf, len, act, paraNo));
    _paragraphs.push_back (std::move(newPara));
}

void EditBufTarget::PutLine (char const * buf, unsigned int len, EditStyle act, int paraNo)
{
	if (!act.IsRemoved ())
		EditBuf::PutLine (buf, len, act, paraNo);
}

void EditBuf::InsertParagraph (int paraNo, std::unique_ptr<EditParagraph> & newPara)
{
	_paragraphs.insert (paraNo, std::move(newPara));
}

void EditBuf::Delete (std::vector<int>::iterator begin, std::vector<int>::iterator end)
{
	while (begin != end)
	{
		_paragraphs.assign_direct (*begin, 0);
		++begin;
	}
	_paragraphs.compact ();
}

void EditBuf::Delete (int paraNo)
{
	_paragraphs.erase (paraNo);
}

void EditBuf::Delete (int firstParaNo, int lastParaNo)
{
	for (int k = firstParaNo; k <= lastParaNo; k++)
		_paragraphs.assign_direct (k, 0);

	_paragraphs.compact ();
}
