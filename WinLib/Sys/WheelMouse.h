#if !defined (WHEELMOUSE_H)
#define WHEELMOUSE_H
//
// (c) Reliable Software 1997, 1998
//
#include <Zmouse.h>     //Support for IntelliMouse under Windows 95 and Win NT 3.51

class IntelliMouse
{
public:
    IntelliMouse ();
    bool IsVScroll (UINT message) const { return message == _msgMouseWheel; }
	bool IsSupported () const { return _msgMouseWheel != 0; }
    int  GetScrollIncrement () const { return _wheelScrollLines; }

private:
	Win::Dow::Handle _hwndWheel;
    UINT    _msgMouseWheel;
    UINT    _msg3DSupport;
    UINT    _msgScrollLines;
    BOOL    _f3DSupport;
    int     _wheelScrollLines;
};

#endif
