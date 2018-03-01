//-----------------------------------
// (c) Reliable Software 1998 -- 2002
//-----------------------------------

#include "precompiled.h"
#include "LineBuf.h"
#include "ListingWin.h"
#include "LineCounter.h"
#include "Crc.h"

void LineBuf::BreakLines ()
{
	Crc::Calc crc;
	if (_buf == 0 || _lenBuf == 0)
		return;
    std::unique_ptr<Line> line (new Line(_buf));
	unsigned long checkSum = 0;
    int begin = 0;
	int i;
    for (i = 0; i < _lenBuf; i++)
    {
        if (_buf [i] == '\n')
        {
            int len = i - begin + 1;
            line->SetLen (len);
            _size += len;
            _lines.push_back (std::move(line));
            begin = i + 1;
			if (begin < _lenBuf)
			{
				std::unique_ptr<Line> newLine (new Line (&_buf [begin]));
				line = std::move(newLine);
			}
        }
        checkSum += _buf [i];
		crc.PutByte (static_cast<unsigned char> (_buf [i]));
    }

	_checkSum.Init (checkSum, crc.Done ());

    if (i - begin > 0)
    {
        int len = i - begin;
        line->SetLen (len);
        _size += len;
        _lines.push_back (std::move(line));
    }
}

void LineBuf::Dump (ListingWindow & listWin, LineCounter & counter, Progress::Meter &)
{
	for (auto_vector<Line>::const_iterator it = _lines.begin (); it != _lines.end (); ++it)
	{
		Line const * line = *it;
        listWin.PutLine (line->Buf (), line->Len (), counter.NextLineNo ());
	}
}
