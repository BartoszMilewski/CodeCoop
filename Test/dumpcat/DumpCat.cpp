//-------------------------------------
// (c) Reliable Software 2000-2003
//-------------------------------------

#include "precompiled.h"
#include "Catalog.h"
#include <Ex/WinEx.h>
#include <Ex/Error.h>
#include <iostream>

// out [patterns]
int main (int count, char * args [])
{
	try
	{
		Catalog catalog;
		catalog.Dump (std::cout);
	}
	catch (Win::Exception e)
	{
		std::cerr << "checkout: " << e.GetMessage () << std::endl;
		SysMsg msg (e.GetError ());
		if (msg)
			std::cerr << "System tells us: " << msg.Text ();
		std::string objectName (e.GetObjectName ());
		if (!objectName.empty ())
			std::cerr << "    " << objectName << std::endl;
	}
	catch (...)
	{
		Win::ClearError ();
		std::cerr << "checkout: Unknown problem\n";
	}
	return 0;
}