//-----------------------------------------
//  SccDllFile.cpp
//  (c) Reliable Software, 1999, 2000, 01
//-----------------------------------------

#include "precompiled.h"
#include "SccDllFile.h"
#include "PathRegistry.h"

#include <File/File.h>
#include <Ex/Winex.h>

char const SccDllFile::SccDllName [] = "SccDll.dll";

SccDllFile::SccDllFile ()
{
	// Check registry key created during setup
	std::string pgmPath = Registry::GetProgramPath ();
	if (!pgmPath.empty ())
	{
		if (File::Exists (pgmPath.c_str ()))
		{
			_programPath.Change (pgmPath);
		}
		else
		{
			throw Win::Exception ("Setup path is not accessible: ", pgmPath.c_str ());
		}
	}
	else
	{
		throw Win::Exception ("Program installation problem: program path is not set.\nYou should reinstall the program.");
	}
}
