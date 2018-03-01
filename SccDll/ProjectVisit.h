#if !defined (PROJECTVISIT_H)
#define PROJECTVISIT_H
//-------------------------------------------------
//  ProjectVisit.h
//  (c) Reliable Software, 2000
//-------------------------------------------------

#include <Dbg/Out.h>
#include <Win/Win.h>
#include <Sys/Synchro.h>

class ServerGate
{
	friend class ProjectVisit;

public:

private:
	Win::Mutex	_gate;
};

class ProjectVisit : public Win::MutexLock
{
public:
	ProjectVisit (ServerGate & serverGate)
		: Win::MutexLock (serverGate._gate)
	{
		//TestingOnly TheLog.Write ("--> LOCK");
	}
	~ProjectVisit ()
	{
		//TestingOnly TheLog.Write ("<-- UNLOCK");
	}
};

extern ServerGate	TheServerGate;

#endif
