#if !defined (LINEBUF_H)
#define LINEBUF_H
//----------------------------------
// (c) Reliable Software 1998 - 2007
//----------------------------------

#include "CluLines.h"
#include "CheckSum.h"

#include <Dbg/Assert.h>

#include <auto_vector.h>

class LineCounter;
class ListingWindow;
namespace Progress { class Meter; }

class LineDumper
{
public:
    virtual void Dump ( ListingWindow & listWin, 
						LineCounter & counter, 
						Progress::Meter & meter) = 0;
};


class LineBuf: public LineDumper
{
public:
    LineBuf (char const * buf, int len)
        : _buf (buf),
		  _lenBuf (len),
		  _lines (512),
		  _size (0)
    {
        BreakLines ();
    }
    Line * GetLine (unsigned int num)
    {
        Assert (num < _lines.size ());
        return _lines [num];
    }
    Line const * GetLine (unsigned int num) const
    {
        Assert (num < _lines.size ());
        return _lines [num];
    }
    unsigned int Count () const { return _lines.size (); }
    unsigned int GetSize () const { return _size; }
    CheckSum GetCheckSum () const { return _checkSum; }
    void Dump (ListingWindow & listWin, LineCounter & counter, Progress::Meter & meter);

private:
    void BreakLines ();

private:
    char const * const  _buf;
    int const           _lenBuf;
    auto_vector<Line>	_lines;
    unsigned int        _size;  // calculated size of file
    CheckSum			_checkSum;
};


#endif
