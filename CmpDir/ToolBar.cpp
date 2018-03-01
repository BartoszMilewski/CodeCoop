// (c) Reliable Software 2002
#include "precompiled.h"
#include "ToolBar.h"

namespace Tool
{
	// In the order in which they appear in the bitmap
	enum ButtonID
	{
		btnNewFile,
		btnDirUp,
		btnDelete,
		btnView,
		btnCut,
		btnPaste,
		btnCopy,
		btnOpen,
		btnRefresh
	};

	Item TheLayout [] = 
	{
		{btnView,		"Selection_View",	"View selected file/folder"},
		{btnDirUp,		"Directory_Up",		"Go up directory"},
		{btnRefresh,	"Directory_Refresh", "Refresh View"},
			{Item::idEnd, 0, 0} 
	};
}