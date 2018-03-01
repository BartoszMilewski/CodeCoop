#if !defined (WATCH_H)
#define WATCH_H
//-----------------------------
//  (c) Reliable Software, 2005
//-----------------------------
#include <File/FolderWatcher.h>

class Watch 
{
public:
	Watch (Win::Dow::Handle win);
	void RemoveFolder ();
	void AddFolder ();
private:
	auto_active<MultiFolderWatcher> _watcher;
	std::vector<std::string> _folders;
};

#endif
