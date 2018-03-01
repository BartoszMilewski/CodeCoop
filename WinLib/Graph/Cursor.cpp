// (c) Reliable Software 2002
#include <WinLibBase.h>
#include <Graph/Cursor.h>

// Warning: Windows defines HCURSOR to be the same as HICON,

template<>
void Win::Disposal<Cursor::Shape>::Dispose (Cursor::Shape h) throw ()
{
	::DestroyCursor (h.ToNative ());
}
