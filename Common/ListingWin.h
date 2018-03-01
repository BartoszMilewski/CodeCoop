#if !defined (LISTINGWIN_H)
#define LISTINGWIN_H
//------------------------------------------------
// ListingWin.h
// (c) Reliable Software 2002
// -----------------------------------------------

#include "EditStyle.h"

class ListingWindow
{
public:
	virtual ~ListingWindow ()
	{}
	virtual void PutLine (char const * buf, unsigned int len, EditStyle act, int targetLineNo) {}
    virtual void PutLine (char const * buf, unsigned int len, int targetLineNo = 0) {}
};

#endif
