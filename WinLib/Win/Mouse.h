#if !defined (MOUSE_H)
#define MOUSE_H
// ---------------------------
// (c) Reliable Software, 2004
// ---------------------------

namespace Mouse
{
	inline unsigned int GetDoubleClickTime () { return ::GetDoubleClickTime (); }
}

#endif
