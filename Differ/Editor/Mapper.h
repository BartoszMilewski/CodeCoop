#if !defined (MAPPER_H)
#define MAPPER_H
//
// (c) Reliable Software, 1997
//

#include <Win/Win.h>
#include <Dbg/Assert.h>

class EditParagraph;

class Tabber
{
public:
	Tabber (int tabLenChar)
		: _tabChar (tabLenChar)
	{}
	void ChangeTabLen (int tabLenChar) { _tabChar = tabLenChar; }
	int SkipTab (int col) const
	{
		int tmp = col + _tabChar - 1;
		Assert (tmp >= 0);
		tmp /= _tabChar;
		return tmp * _tabChar;
	}
	int ExpandTabs (EditParagraph const * para, int start, int len, int startCol) const;
private:
	int _tabChar;
};

// Maps pixels into (line, col) positions and vice versa
class ColMapper
{
public:
    ColMapper (Win::Dow::Handle hwnd, Font::Handle font);

	void Init (int cx, int cy)
	{
		_cx = cx;
		_cy = cy;
	}

    int ColWidth ()  const { return _cx; }
    int LineHeight () const { return _cy; }

    int ColToX (int col)  const { return col * _cx; }
    int LineToY (int line) const { return line * _cy; }

    int XtoColTrunc  (int x) const { return x / _cx; }
    int YtoLineTrunc (int y)  const { return y / _cy; }

    int XtoColRound (int x) const { return (x + _cx - 1)/_cx; }
    int YtoLineRound (int y) const { return (y + _cy - 1)/_cy; }

    int XtoCol (int x) const { return (x + (_cx - 1)/2 ) / _cx; }
    int YtoLine (int y) const { return (y + (_cy - 1)/2 ) / _cy; }

private:
    int     _cx;
    int     _cy;
};

#endif
