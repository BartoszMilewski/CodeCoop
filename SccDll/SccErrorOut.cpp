//-----------------------------------------
//  (c) Reliable Software, 2003
//-----------------------------------------

#include "precompiled.h"
#include "SccErrorOut.h"

#include <windows.h>
#include "Scc.h"

#include <iostream>

long SccErrorOut (char const * msg, unsigned long msgType)
{
	switch (msgType)
	{
	case SCC_MSG_INFO:		// Message is informational
		std::cerr << "Information: ";
		break;
	case SCC_MSG_WARNING:	// Message is a warning
		std::cerr << "Warning: ";
		break;
	case SCC_MSG_ERROR:		// Message is an error
		std::cerr << "Error: ";
		break;
	case SCC_MSG_STATUS:	// Message is meant for status bar
		std::cerr << "Status: ";
		break;
	default:
		break;
	}
	std::cerr << msg << std::endl;
	return SCC_OK;
}

