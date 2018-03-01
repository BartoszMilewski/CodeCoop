#if !defined (LINECOUNTER_H)
#define LINECOUNTER_H
//-----------------------------------
// Reliable Software (c) 1998 -- 2002
//-----------------------------------

#include "EditStyle.h"

class LineCounter
{
public:
	LineCounter ()
		: _lineNo (0)
	{}
	virtual ~LineCounter () {}

	virtual void SetMode (EditStyle::Action act, int newLineNo) {}
	virtual int NextLineNo () 
	{ 
		return _lineNo++; 
	}
protected:
	int		_lineNo;
};

class LineCounterDiff: public LineCounter
{
public:
	void SetMode (EditStyle::Action act, int newLineNo);
	int NextLineNo ();
protected:
	bool	_doIncrement;
};

class LineCounterOrig: public LineCounterDiff
{
public:
	void SetMode (EditStyle::Action act, int newLineNo);
};

class LineCounterMerge: public LineCounterDiff
{
public:
	void SetMode (EditStyle::Action act, int newLineNo);
private:
};

#endif
