#if !defined (ACTIVEMERGER_H)
#define ACTIVEMERGER_H
//------------------------------------
//  (c) Reliable Software, 2006 - 2007
//------------------------------------

#include "GlobalId.h"
#include "IpcExchange.h"

#include <Sys/Active.h>
#include <Win/Message.h>
#include <Sys/Synchro.h>
#include <auto_vector.h>
#include <XML/XmlTree.h>

namespace XML { class Tree; }

class ActiveMerger : public ActiveObject
{
public:
	ActiveMerger (Win::Dow::Handle winNotify, 
		XML::Tree & xmlArgs, 
		std::string const & appPath, 
		GlobalId gid);

	GlobalId GetFileGid () const { return _mergedFileGid; }

private:
	void Run ();
	void FlushThread () { _event.Release (); }

private:
	Win::Event				_event;
	Win::Dow::Handle		_winNotify;
	XML::Tree				_xmlArgs;
	XmlBuf					_sharedXmlBuf;
	std::string				_appPath;
	Win::RegisteredMessage	_msgActiveMerger;
	GlobalId				_mergedFileGid;
};

class ActiveMergerWatcher
{
public:
	bool IsEmpty () const { return _mergers.size () == 0; }
	void Clear () { _mergers.clear (); }

	void Add (XML::Tree & xmlArgs, std::string const & appPath, GlobalId fileGid);

	GlobalId FindErase (ActiveMerger const * merger)
	{
		GlobalId gid = gidInvalid;
		Iterator it;
		for (it = _mergers.begin(); it != _mergers.end (); ++it)
		{
			if ((*it)->get () == merger)
				break;
		}
			
		if (it != _mergers.end ())
			gid = merger->GetFileGid ();
		_mergers.erase (_mergers.ToIndex (it));
		return gid;
	}

private:
	auto_vector<auto_active<ActiveMerger> > _mergers;
	typedef auto_vector<auto_active<ActiveMerger> >::iterator Iterator;
};

#endif
