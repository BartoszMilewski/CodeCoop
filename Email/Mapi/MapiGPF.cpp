// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------
#include "precompiled.h"
#include "MapiGPF.h"
#include "OutputSink.h"

char const Err [] = "Unexpected error in MAPI code during a call to ";

void Mapi::HandleGPF (std::string const & where)
{
	std::string msg = Err + where + ".";
	TheOutput.Display (msg.c_str ());
}
