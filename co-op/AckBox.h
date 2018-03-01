#if !defined (ACKBOX_H)
#define ACKBOX_H
//------------------------------------
//  (c) Reliable Software, 2004 - 2007
//------------------------------------

#include "GlobalId.h"
#include "Lineage.h"

class ScriptMailer;
class ScriptHeader;
class CommandList;
namespace Project
{
	class Db;
}
namespace CheckOut
{

	class List;
}

class AckBox
{
public:
	bool empty () const { return _mustAck.empty (); }
	void RememberMakeRef (Unit::ScriptId const & acknowledgedScriptId);
	void RememberMakeRef (UserId recipientId, Unit::ScriptId const & acknowledgedScriptId);
	void RememberAck (UserId recipientId, Unit::ScriptId const & acknowledgedScriptId, bool mustAcknowledge = true);
	void RemoveAckForScript (UserId recipientId, Unit::ScriptId const & acknowledgedScriptId);
	void XSendAck (ScriptMailer & mailer,
				   Project::Db & projectDb,
				   ScriptHeader & ackHdr,
				   CheckOut::List const * checkoutNotification) const;

private:
	struct AckKey
	{
		AckKey (UserId recipientId, Unit::Type unitType)
			: _recipientId (recipientId),
				_unitType (unitType)
		{}

		bool operator< (AckKey const & key) const
		{
			if (key._recipientId < _recipientId)
				return true;
			else if (key._recipientId == _recipientId)
				return key._unitType < _unitType;
			else
				return false;
		}

		UserId		_recipientId;
		Unit::Type	_unitType;
	};

	struct AckScriptId
	{
		AckScriptId (GlobalId scriptId, bool makeRef)
			: _scriptId (scriptId),
				_makeRef (makeRef)
		{}

		GlobalId	_scriptId;
		bool		_makeRef;
	};

	class IsEqualScriptId : public std::unary_function<AckScriptId const *, bool>
	{
	public:
		IsEqualScriptId (AckScriptId id)
			: _id (id)
		{}

		bool operator () (AckScriptId const & id) const
		{
			return _id._scriptId == id._scriptId && _id._makeRef == id._makeRef;
		}

	private:
		AckScriptId	_id;
	};

	typedef std::vector<AckScriptId> AckList;

	typedef std::map<AckKey, AckList> AckRecipients;
	typedef std::pair<AckKey, AckList> AckRecipient;

	bool IsPresent (AckRecipients const & list, UserId recipientId, Unit::ScriptId const & acknowledgedScriptId, bool makeRef = false) const;
	void AddAck (AckRecipients & list, UserId recipientId, Unit::ScriptId const & acknowledgedScriptId, bool makeRef = false);
	void AddAckCmds (CommandList & cmdList, AckList const & ackList) const;
	void UpdateScriptComment (std::string & comment, AckList const & ackList) const;

private:
	AckRecipients	_mustAck;
	AckRecipients	_canAck;
};

#endif
