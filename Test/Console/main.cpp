#include "precompiled.h"
#include <iostream>
#include <Ctrl/Output.h>
#include <Sys/Dll.h>
#include <Com/com.h>
#include <Net/NetShare.h>
#include <Net/SharedObject.h>

Out::Sink TheOutput;

// Warning: This program runs as administrator
int main (int argc, char * argv [])
{
	try // to change network sharing
	{
		Net::Share share;
		Net::SharedFolder folder ("CODECOOP", "c:\\co-op\\PublicInbox", "Code Co-op Share");
		share.Add (folder);
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
		Win::ClearError ();
	}

	return 0;
}

