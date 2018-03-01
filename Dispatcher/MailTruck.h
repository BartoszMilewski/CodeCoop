#if !defined (MAILTRUCK_H)
#define MAILTRUCK_H
//----------------------------------
// (c) Reliable Software 1998 - 2002
//----------------------------------
#include "Transport.h"
#include "ScriptInfo.h"

class ConfigData;
class WorkQueue;
class FilePath;
class Transport;
class LocalProjects;

class MailTruck
{
public:
    MailTruck (ConfigData const & config, LocalProjects & localProjects);
	ScriptTicket & PutScript (std::unique_ptr<ScriptTicket> script);

    void CopyLocal (ScriptTicket & script,
					int addresseeIdx,
					int projectId,
					FilePath const & destPath);

	bool GetDoneScripts (ScriptVector & doneList);
    void Distribute (WorkQueue & workQueue);
	bool HasHub() const { return _topology.IsSatellite() || _topology.IsRemoteSatellite(); }
	void RemoveScript(ScriptTicket const & script);
private:
	ScriptVector	_scriptTicketList;

	LocalProjects &		_localProjects;

	Topology    const   _topology;
	FilePath	const & _pathToPublicInbox;
    Transport	const & _hubTransport;
	std::string const & _hubEmail;
};

#endif
