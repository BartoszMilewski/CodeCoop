#if !defined (ACTIVITYLOG_H)
#define ACTIVITYLOG_H
//------------------------------
// (c) Reliable Software 2005
//------------------------------
#include <File/Path.h>

#include <iosfwd>

namespace Mailbox
{
	class ScriptInfo;
}

class ActivityLog
{
public:
	ActivityLog (FilePath const & logsDir)
		: _logsDir (logsDir)
	{}

	void ReceiverLicense (std::string const & licensee);
	void CorruptedScript (Mailbox::ScriptInfo const & info);

	void Dump (std::ofstream & out) const;

private:
	void DumpLog (char const * logPath, std::string const & caption, std::ofstream & out) const;

private:
	static char const *	_receiverLicenses;
	static char const *	_corruptedScripts;

private:
	FilePath	_logsDir;
};

#endif
