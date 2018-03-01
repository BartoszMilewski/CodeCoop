#if !defined (MEMBERSHIPCHANGE_H)
#define MEMBERSHIPCHANGE_H
// ----------------------------------
// (c) Reliable Software, 2006 - 2007
// ----------------------------------

#include "ScriptCommandList.h"

namespace Project
{
	class Db;
}

namespace History
{
	class Db;
}

namespace CheckOut
{
	class List;
}

class MemberInfo;
class ScriptHeader;
class ScriptMailer;
class AckBox;
class DispatcherCmd;

class XMembershipChange
{
public:
	XMembershipChange (MemberInfo const & updateInfo, Project::Db & projectDb);

	bool ChangesDetected () const { return _changesDetected; }
	void UpdateProjectMembership ();
	void BuildScript (History::Db & history);
	void StoreInHistory (History::Db & history, AckBox & ackBox) const;
	void Broadcast (ScriptMailer & mailer,
					std::unique_ptr<DispatcherCmd> attachment,
					CheckOut::List const * notification);
	void BuildFutureDefect (ScriptMailer & mailer, 
							History::Db & history,
							std::string & defectFilename,
							std::vector<unsigned char> & defectScript);
private:
	std::string BuildComment () const;
private:
	MemberInfo const &			_newMemberInfo;
	std::unique_ptr<MemberInfo>	_oldMemberInfo;
	Project::Db &				_projectDb;
	bool						_sendScript;
	bool						_changesDetected;
	std::unique_ptr<ScriptHeader>	_hdr;
	CommandList					_cmdList;
};

#endif
