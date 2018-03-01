#if !defined (CLIPBOARDTEXT_H)
#define CLIPBOARDTEXT_H
//
// (c) Reliable Software, 1998
//

#include "lines.h"
#include "LineBuf.h"

#include <Sys/Clipboard.h>
#include <Sys/WinGlobalMem.h>

class ClipboardText: public EditBuf
{
public:
	ClipboardText (Clipboard const & clipboard)
		: _buf (clipboard.GetText ())
	{
		LineBuf textLines (&_buf [0], strlen (&_buf [0]));
		Init (textLines);
	}

private:
	GlobalBuf	_buf;
};

#endif
