#if !defined (MAILBOXHELPER_H)
#define MAILBOXHELPER_H
//----------------------------------
// (c) Reliable Software 2004 - 2007
//----------------------------------

#include "MailboxScriptInfo.h"
#include "Agents.h"
#include "ActivityLog.h"
#include "UniqueName.h"
#include "CheckoutNotifications.h"

#include <auto_vector.h>

class ScriptHeader;
class Catalog;
class ScriptMailer;
class CommandList;
class MemberDescription;
class DispatcherScript;
class TransactionFileList;
class CtrlScriptProcessor;
class AckBox;
class DataBase;

namespace History
{
	class Db;
}

namespace Mailbox
{
	class Db;

	class Agent
	{
	public:
		Agent (TransactionFileList & unpackedScripts,
			   Permissions & userPermissions,
			   AckBox & ackBox,
			   UserId thisUserId,
			   ActivityLog & activityLog)
			: _activityLog (activityLog),
			  _thisUserAgent (userPermissions),
			  _ackBox (ackBox),
			  _unpackedScripts (unpackedScripts),
			  _curScriptId (gidInvalid),
			  _knownMissingId (gidInvalid),
			  _knownMissingUnitId (gidInvalid),
			  _myId (thisUserId),
			  _atLeastOneScriptUnpacked (false),
			  _isPendingWork (false),
			  _fullSyncCorruptionDetected (false),
			  _deleteProjectVerificationMarker (false)
		{}

		void XFinalize (Mailbox::Db & mailbox,
						History::Db & history,
						DataBase & database,
						CheckOut::Db & checkoutFiles,
						CtrlScriptProcessor & ctrlScriptProcessor,
						CheckOut::List const * notification);
		void XRefreshUserData (Project::Db const & projectDb);
		void DeleteScriptFile (std::string const & path);
		ThisUserAgent & GetThisUserAgent () { return _thisUserAgent; }
		AckBox & GetAckBox () { return _ackBox; }
		void RememberCtrlScript (ScriptInfo const & info) { _ctrlScripts.push_back (info); }
		void RememberExecuteOnlyScript (ScriptInfo const & info) { _executeOnlyScripts.push_back (info); }
		void RememberNewFolder (GlobalId gid, UniqueName const & uName);
		void RememberCheckoutNotification (std::unique_ptr<CheckOut::List> notification)
		{
			if (notification->GetSenderId () != _myId) 
				_checkoutNotifications.push_back (std::move(notification));
		}
		void DeletePrehistoricScript (GlobalId scriptId, std::string const & path)
		{
			_prehistoricScripts.push_back (scriptId);
			DeleteScriptFile (path);
		}
		void RememberMissingScriptPlaceholder (GlobalId missingId, Unit::Type unitType)
		{
			_newMissingPlaceholders.push_back (std::make_pair (missingId, unitType));
		}
		void SetCurrentScript (GlobalId scriptId)
		{
			_curScriptId = scriptId;
			_knownMissingId = gidInvalid;
			_knownMissingUnitId = gidInvalid;
		}
		void SetKnownMissingScript (GlobalId scriptId, GlobalId unitId)
		{
			if (GlobalIdPack (scriptId).GetUserId () != _myId)
			{
				_knownMissingId = scriptId; 
				_knownMissingUnitId = unitId;
			}
		}
		void SetScriptUnpacked (bool flag) { _atLeastOneScriptUnpacked = flag; }
		void SetPendingWork (bool flag) { _isPendingWork = flag; }
		void SetDeleteProjectVerificationMarker (bool flag) { _deleteProjectVerificationMarker = flag; }

		bool IsFullSyncCorruptionDetected () const { return _fullSyncCorruptionDetected; }
		bool IsDeleteProjectVerificationMarker () const { return _deleteProjectVerificationMarker; }
		bool HasKnownMissingId () const { return _knownMissingId != gidInvalid; }
		bool HasCtrlScripts () const { return !_ctrlScripts.empty (); }
		bool HasPrehistoricScripts () const { return !_prehistoricScripts.empty (); }
		bool IsPendingWork () const { return _atLeastOneScriptUnpacked && _isPendingWork; }
		unsigned int CtrlScriptCount () const { return _ctrlScripts.size (); }

		GlobalId GetKnownMissingId () const { return _knownMissingId; }
		GlobalId GetKnownMissingUnitId () const { return _knownMissingUnitId; }
		UserId GetIncomingScriptSenderId () const { return GlobalIdPack (_curScriptId).GetUserId (); }
		UserId GetMyId () const { return _myId; }

		void LogCorruptedScript (ScriptInfo const & info)
		{
			if (info.IsFullSynch ())
				_fullSyncCorruptionDetected = true;
#if 0
			_activityLog.CorruptedScript (info);
#endif
		}

	private:
		typedef std::vector<std::pair<GlobalId, Unit::Type> > MissingList;
		typedef std::vector<std::pair<GlobalId, UniqueName> > NewFolderList;

		ActivityLog	&			_activityLog;
		ThisUserAgent 			_thisUserAgent;
		AckBox &				_ackBox;
		TransactionFileList &	_unpackedScripts;
		std::vector<ScriptInfo> _ctrlScripts;
		std::vector<ScriptInfo> _executeOnlyScripts;
		auto_vector<CheckOut::List>	_checkoutNotifications;
		GidList					_prehistoricScripts;
		MissingList				_newMissingPlaceholders;
		NewFolderList			_newFolders;
		GlobalId				_curScriptId;
		GlobalId				_knownMissingId;
		GlobalId				_knownMissingUnitId;
		UserId					_myId;
		bool					_atLeastOneScriptUnpacked;
		bool					_isPendingWork;
		bool					_fullSyncCorruptionDetected;
		bool					_deleteProjectVerificationMarker;
	};
}

#endif
