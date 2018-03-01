#if !defined (SCRTIPTBASKET_H)
#define SCRTIPTBASKET_H
//---------------------------
// (c) Reliable Software 2009
//---------------------------
#include "GlobalId.h"
#include "ScriptHeader.h"
#include "ScriptCommandList.h"
class ScriptMailer;
namespace CheckOut { class List; }
namespace Project { class Db; }

class ScriptBasket
{
	struct ScriptPack
	{
		ScriptPack(std::unique_ptr<ScriptHeader> hdr, CommandList & cmdList, UserIdList const & userIds)
			: _userIdList(userIds), _hdr(std::move(hdr))
		{
			_cmdList.swap(cmdList);
		}
		std::unique_ptr<ScriptHeader> _hdr;
		CommandList _cmdList;
		UserIdList  _userIdList;
	};
public:
	void SendScripts(ScriptMailer & mailer, CheckOut::List const * notifications, Project::Db const & projectDb);
	void Put(std::unique_ptr<ScriptHeader> hdr, CommandList & cmdlist, UserId userId);
	void Put(std::unique_ptr<ScriptHeader> hdr, CommandList & cmdlist, UserIdList const & userIds);
private:
	typedef auto_vector<ScriptPack>::const_iterator PackIter;
	auto_vector<ScriptPack> _scriptPacks;
};

#endif
