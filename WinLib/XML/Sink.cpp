// ---------------------------
// (c) Reliable Software, 2003
// ---------------------------
#include "WinLibBase.h"
#include "Sink.h"

namespace XML
{
	char const * Error::_errorMsg [] =
	{
		"Expected end of file",
		"Expected XML declaration tag",
		"Expected start of tag",
		"Expected end of tag",
		"End tag not closed"
	};

	Error::Error (ErrorCode errorCode)
		:_err (errorCode)
	{
		for (unsigned int i = 0; i < Count; ++i)
		{
			Assert (_errorMsg [i] != 0);
		}
	}

}
