#if !defined (EMAILCHECKER_H)
#define EMAILCHECKER_H
//---------------------------
// (c) Reliable Software 2010
//---------------------------
#include <Sys/Active.h>
#include <Sys/Channel.h>
#include <File/Path.h>

class EmailChecker: public ActiveObject
{
public:
	EmailChecker(Win::Dow::Handle winParent, FilePath const & publicInboxPath)
		: _winParent(winParent), _publicInboxPath(publicInboxPath)
	{}
	void Run();
	void FlushThread() { _sync.Release(); }
	void Force() { _sync.Release(); }

private:
	bool RetrieveEmail (bool isVerbose);
	static bool UnpackFile (
		std::string const & fromPath,
		std::string const & extension,
		std::string const & emailSubject,
		FilePath const & mailDest);

private:
	Win::Dow::Handle 	_winParent;
	FilePath			_publicInboxPath;
	Win::Event 			_sync;
};

#endif
