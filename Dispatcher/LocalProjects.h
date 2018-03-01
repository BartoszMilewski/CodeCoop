#if !defined (LOCALPROJECTS_H)
#define LOCALPROJECTS_H
// ---------------------------
// (c) Reliable Software, 2002
// ---------------------------

#include "GlobalId.h"
#include <File/Dir.h>
#include <set>

class Catalog;

class LocalProjects
{
public:
	LocalProjects (Catalog & catalog);
	void Refresh ();

	// incoming scripts: must be reported to Co-op
	void MarkNewIncomingScripts (int projectId);
	bool AreNewWithIncomingScripts () const { return _areNewWithIncoming; }
	bool UnpackIncomingScripts ();
	void ResendMissingScripts ();

	// feedback from Co-op: projects with scripts to be synchronized
	bool OnStateChange (int projectId);
	int  GetToBeSynchedCount () const { return _projectsToSync.size (); }
private:
	Catalog			  &	_catalog;

	FileMultiSeq::Patterns _scriptPattern;

	std::set<int>	_projectsToSync; // projects with scripts received by co-op and waiting
										 // to be synched (incoming scripts marker)
	std::set<int>	_projectsWithIncoming; // projects with script files in inboxes
											   // (physical lookup of inboxes)
	std::set<int>	_projectsWithMissing;
	bool			_areNewWithIncoming;
};

#endif
