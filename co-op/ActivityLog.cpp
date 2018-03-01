//----------------------------------
// (c) Reliable Software 2005 - 2007
//----------------------------------
#include "precompiled.h"
#include "ActivityLog.h"
#include "MailboxScriptInfo.h"

#include <TimeStamp.h>
#include <File/File.h>
#include <File/FileIo.h>

char const * ActivityLog::_receiverLicenses = "ReceiverLicenses.txt";
char const * ActivityLog::_corruptedScripts = "CorruptedScripts.txt";

void ActivityLog::ReceiverLicense (std::string const & licensee)
{
	std::string fullPath = _logsDir.GetFilePath (_receiverLicenses);
	OutStream logFile (fullPath, std::ios_base::app);	// append
	if (!logFile.is_open ())
	{
		File::CreateFolder (_logsDir.GetDir ());
		logFile.Open (fullPath, std::ios_base::app); // append
		if (!logFile.is_open ())
			return; // quiet failure
	}
	StrTime timeStamp (CurrentTime ());
	logFile << timeStamp.GetString () << " " << licensee << std::endl;
}

void ActivityLog::CorruptedScript (Mailbox::ScriptInfo const & info)
{
	// Revisit: synchronize log output between multiple Co-op instances.
	// Provide generic interface for activity logging (for example std::ofstream)
#if 0
	OutStream logFile;
	std::string fullPath = _logsDir.GetFilePath (_corruptedScripts);
	logFile.open (fullPath.c_str (), std::ios_base::app); // append
	if (!logFile.is_open ())
	{
		File::CreateFolder (_logsDir.GetDir ());
		logFile.open (fullPath.c_str (), std::ios_base::app); // append
		if (!logFile.is_open ())
			return; // quiet failure
	}
	StrTime timeStamp;
	logFile << std::endl << timeStamp.GetString () << std::endl;
	logFile << info << std::endl;
#endif
}

void ActivityLog::Dump (std::ofstream & out) const
{
	DumpLog (_logsDir.GetFilePath (_receiverLicenses), "Receiver licenses:", out);
	DumpLog (_logsDir.GetFilePath (_corruptedScripts), "Corrupted scripts:", out);
}

void ActivityLog::DumpLog (char const * logPath, std::string const & caption, std::ofstream & out) const
{
	if (File::Exists (logPath))
	{
		InStream logFile (logPath);
		if (logFile.is_open ())
		{
			out << std::endl << caption << std::endl;
			// Append log to the output file	
			while (!logFile.eof ())
			{
				char line [512];
				logFile.getline (line, sizeof (line));
				out << line << std::endl;
			}
			out << std::endl;
		}
	}
}
