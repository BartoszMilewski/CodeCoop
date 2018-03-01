//----------------------------------
// (c) Reliable Software 2004 - 2007
//----------------------------------

#include "precompiled.h"
#include "MemoryLog.h"

#include <Com/Shell.h>
#include <File/Path.h>
#include <File/FileIo.h>

void MemoryLog::Save (std::string const & path)
{
	OutStream out;
	out.open (path.c_str ());
	if (out.good ())
	{
		for (unsigned int i = 0; i <_log.size (); i++)
		{
			out << _log [i];
		}
		out.close ();
	}
}

int MemoryLog::OutputBuf::sync ()
{
	char * pch = pptr();
	if (pch != 0)
	{
		//	We've got data in the buffer
		*pch = '\0';
		_log.push_back (_buffer);
	}
	doallocate ();
	return 0;
}

int MemoryLog::OutputBuf::overflow (int ch)
{
	sync ();
	if (ch != EOF)
	{
		_buffer [0] = static_cast<char> (ch);
		pbump (1);
	}
	return 0;
}

int MemoryLog::OutputBuf::doallocate ()
{
	setp (_buffer, _buffer + BufferSize);
	return BufferSize;
}

void DbgTracer::SaveLog ()
{
	FilePath userDesktopPath;
	ShellMan::VirtualDesktopFolder userDesktop;
	userDesktop.GetPath (userDesktopPath);
	MemoryLog::Save (userDesktopPath.GetFilePath (_logFileName));
}
