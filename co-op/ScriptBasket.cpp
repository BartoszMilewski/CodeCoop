//---------------------------
// (c) Reliable Software 2009
//---------------------------
#include "precompiled.h"
#include "ScriptBasket.h"
#include "Mailer.h"
#include "ProjectDb.h"
#include "Addressee.h"

void ScriptBasket::SendScripts(ScriptMailer & mailer, CheckOut::List const * notifications, Project::Db const & projectDb)
{
	for (PackIter it = _scriptPacks.begin(); it != _scriptPacks.end(); ++it)
	{
		ScriptPack const * pack = *it;
		AddresseeList addrList;
		UserIdList const & uids = pack->_userIdList;
		for (UserIdList::const_iterator uidIter = uids.begin(); uidIter != uids.end(); ++uidIter)
		{
			std::unique_ptr<MemberDescription> member = projectDb.RetrieveMemberDescription(*uidIter);
			Addressee addressee (member->GetHubId (), member->GetUserId ());
			addrList.push_back (addressee);
		}
		mailer.Multicast(*(pack->_hdr), pack->_cmdList, addrList, notifications);
	}
}

void ScriptBasket::Put(std::unique_ptr<ScriptHeader> hdr, CommandList & cmdList, UserId userId)
{
	UserIdList userIds;
	userIds.push_back(userId);
	Put(std::move(hdr), cmdList, userIds);
}

void ScriptBasket::Put(std::unique_ptr<ScriptHeader> hdr, CommandList & cmdList, UserIdList const & userIds)
{
	std::unique_ptr<ScriptPack> pack(new ScriptPack(std::move(hdr), cmdList, userIds));
	_scriptPacks.push_back(std::move(pack));
}

