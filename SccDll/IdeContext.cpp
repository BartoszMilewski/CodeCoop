//-----------------------------------------
//  (c) Reliable Software, 2000 -- 2003
//-----------------------------------------

#include "precompiled.h"
#include "IdeContext.h"
#include <windows.h>
#include "Scc.h"
#include <StringOp.h>

IDEContext::IDEContext (char const * callerName)
	: _version (0),
	  _isBorlandIDE (false),
	  _projectId (-1),
	  _ideTextOut (0)
{
//	if (callerName != 0)
//		_isBorlandIDE = IsNocaseEqual (callerName, "TSCCProvider");
}

void IDEContext::Display (std::string const & msg, bool isError) const
{
	if (_ideTextOut != 0)
	{
		_ideTextOut (msg.c_str (), isError ? SCC_MSG_ERROR : SCC_MSG_INFO);
	}
}