#if !defined (CTRLSCRIPTPROCESSOR_H)
#define CTRLSCRIPTPROCESSOR_H
//----------------------------------
// (c) Reliable Software 2004 - 2007
//----------------------------------
#include "GlobalId.h"

class ScriptHeader;
class CommandList;
class ScriptMailer;
class Sidetrack;
class AckBox;
namespace Project
{
	class Db;
}
namespace History
{
	class Db;
}
namespace CheckOut { class List; }

class CtrlScriptProcessor
{
public:
	CtrlScriptProcessor (ScriptMailer & mailer, Project::Db & projectDb, Sidetrack & sidetrack)
		: _mailer (mailer),
		  _projectDb (projectDb),
		  _sidetrack (sidetrack)
	{}

	bool NeedToForwardJoinRequest () const { return !_joinRequestPath.empty (); }
	std::string const & GetJoinRequestPath () const { return _joinRequestPath; }

	void XRememberJoinRequest (std::string const & path) { _joinRequestPath = path; }
	bool XRememberResendRequest (ScriptHeader const & hdr, CommandList const & cmdList);
	void XExecuteScript (ScriptHeader const & hdr, 
		CommandList const & cmdList, 
		History::Db & history, 
		AckBox & ackBox,
		CheckOut::List const * notification);
private:
	bool XIsMemberReceiver (UserId uid) const;
private:
	ScriptMailer &	_mailer;
	Project::Db &	_projectDb;
	Sidetrack &		_sidetrack;
	std::string		_joinRequestPath;
};

#endif
