#if !defined SCRIPTMAILER_H
#define SCRIPTMAILER_H
//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "GlobalId.h"

class DispatcherScript;
class MemberDescription;
class AddresseeList;
namespace Project
{
	class Db;
}
class Catalog;
class TransportHeader;
class FilePath;
class Addressee;
class ScriptHeader;
class ScriptTrailer;
class CommandList;
class ScriptList;
class ScriptBuilder;
class TransportHeader;
namespace CheckOut
{
	class List;
}

// Revisit:
// Scripts should NOT be sent under transaction. If the transaction aborts
// after the script is sent, all hell breaks loose!

class ScriptMailer
{
public:
	ScriptMailer (Project::Db const & projectDb, Catalog & catalog)
		: _projectDb (projectDb),
		  _catalog (catalog)
    {}

    void XBroadcast (ScriptHeader & hdr,
					 CommandList const & cmdList, 
					 CheckOut::List const * checkOutNotifications);
    void Broadcast (ScriptHeader & hdr,
					CommandList const & cmdList, 
					CheckOut::List const * checkOutNotifications);
	std::string BroadcastForcedDefect (ScriptHeader & hdr,
									   CommandList const & cmdlist,
									   FilePath & scriptPath,
									   int projectId);
    void Multicast (ScriptHeader & hdr,
					CommandList const & cmdlist,
					AddresseeList const & addresseeList,
					CheckOut::List const * notification,
					DispatcherScript const * dispatcher = 0);
    void XMulticast (ScriptHeader & hdr,
					 CommandList const & cmdlist,
					 GidSet const & filterOut,
					 CheckOut::List const * notification,
					 DispatcherScript const * dispatcher = 0);
	void XFutureMulticast (ScriptHeader & hdr,
						   CommandList const & cmdList,
						   GidSet const & filterOut,
						   DispatcherScript const & dispatcherScript,
						   UserId senderId,
						   std::string & scriptFilename,
						   std::vector<unsigned char> & script);
    void XUnicast (ScriptHeader & hdr,
				   CommandList const & cmdList,
				   MemberDescription const & recipient,
				   CheckOut::List const * notification,
				   DispatcherScript const * dispatcher = 0);
    void XUnicast (ScriptHeader & hdr,
				   ScriptList const & scriptList,
				   MemberDescription const & recipient,
				   CheckOut::List const * notification,
				   ScriptTrailer const * trailer = 0,
				   DispatcherScript const * dispatcher = 0);
    void XUnicast (std::vector<unsigned> const & chunkList, 
				   ScriptHeader & hdr,
				   ScriptList const & scriptList,
				   MemberDescription const & recipient,
				   CheckOut::List const * notification);
	void UnicastJoin (ScriptHeader & hdr,
					  CommandList const & cmdList,
					  MemberDescription const & sender,
					  MemberDescription const & admin,
					  DispatcherScript const * dispatcherScript);
	void ForwardJoinRequest (std::string const & scriptPath);

	std::string XSaveInTmpFolder (ScriptHeader & hdr,
								  ScriptList const & scriptList,
								  MemberDescription const & recipient,
								  ScriptTrailer const & trailer,
								  DispatcherScript const & dispatcher);

private:
	bool BuildTxHdr (TransportHeader & txHdr,
					 DispatcherScript const * dispatcherScript);
	bool XBuildTxHdr (TransportHeader & txHdr,
					  DispatcherScript const * dispatcherScript);
	void SaveSingleRecipientScripts (ScriptBuilder & bilder,
									 TransportHeader & txHdr,
									 AddresseeList const & addresseeList);

private:
	Project::Db const & _projectDb;
	Catalog &			_catalog;
};

#endif
