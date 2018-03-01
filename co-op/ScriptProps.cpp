//----------------------------------
// (c) Reliable Software 2002 - 2007
// ---------------------------------

#include "precompiled.h"
#include "ScriptProps.h"
#include "ScriptHeader.h"
#include "ScriptIo.h"
#include "ScriptCmd.h"
#include "ProjectDb.h"
#include "UserIdPack.h"
#include "MemberInfo.h"
#include "Workspace.h"
#include "PathFind.h"
#include "AppInfo.h"
#include "DisplayWin.h"
#include "Registry.h"
#include "TransportHeader.h"
#include "Sidetrack.h"

#include <Win/Metrics.h>
#include <Win/Geom.h>

#include <StringOp.h>
#include <TimeStamp.h>

class ScriptDisplayTable : public FileDisplayTable
{
public:
	ScriptDisplayTable (PathFinder const & pathFinder)
		: _pathFinder (pathFinder),
		  _placementOffset (20)
	{}

	void Init (std::unique_ptr<CommandList> cmdList);
	//
	// FileDisplayTable interface
	//
	GidSet const & GetFileSet () const { return _files; }
	void Open (GidList const & gids);
	void OpenAll ();
	void Cleanup (GlobalId gid) {};
	char const * GetRootRelativePath (GlobalId gid) const;
	char const * GetFileName (GlobalId gid) const;
	FileType GetFileType (GlobalId gid) const;
	bool IsFolder (GlobalId gid) const;
	FileDisplayTable::ChangeState GetFileState (GlobalId gid) const;

private:
	typedef std::map<GlobalId, Workspace::ScriptItem const *>::const_iterator ItemIterator;
	typedef std::map<GlobalId, Win::Dow::Handle>::const_iterator WinIterator;

private:
	std::unique_ptr<CommandList>							_cmdList;	// Script command list
	std::unique_ptr<Workspace::ScriptSelection>			_selection;
	std::map<GlobalId, Workspace::ScriptItem const *>	_items;		// For quick lookup in the selection
	GidSet												_files;
	PathFinder const &									_pathFinder;
	int													_placementOffset;
	std::map<GlobalId, Win::Dow::Handle>				_openWins;	// For quick lookup of open dump windows
	Win::Dow::Handle									_lastOpenDumpWin;
};

void ScriptDisplayTable::Init (std::unique_ptr<CommandList> cmdList)
{
	_cmdList = std::move(cmdList);
	_selection.reset (new Workspace::ScriptSelection (*_cmdList));
	for (Workspace::Selection::Sequencer seq (*_selection); !seq.AtEnd (); seq.Advance ())
	{
		Workspace::ScriptItem const & item = seq.GetScriptItem ();
		GlobalId gid = item.GetItemGid ();
		_files.insert (gid);
		_items [gid] = &item;
	}
	NonClientMetrics metrics;
	if (metrics.IsValid ())
		_placementOffset = metrics.GetBorderWidth () + metrics.GetCaptionHeight ();
}

void ScriptDisplayTable::Open (GidList const & gids)
{
	WinIterator winIter = _openWins.end ();
	for (GidList::const_iterator iter = gids.begin (); iter != gids.end (); ++iter)
	{
		GlobalId gid = *iter;
		winIter = _openWins.find (gid);
		if (winIter != _openWins.end ())
		{
			// Check if dump window is still open
			// Bring to top already open dump window
			Win::Dow::Handle win (winIter->second);
			if (win.Exists ())
				win.BringToTop ();	// Bring to top already open dump window
			else
				winIter = _openWins.end ();	// Open again dump window
		}
	}

	if (winIter == _openWins.end ())
	{
		// Open new dump window
		std::string caption;
		if (gids.size () == 1)
		{
			caption += GetRootRelativePath (gids [0]);
		}
		else
		{
			caption += "Showing changes in ";
			caption += ToString (gids.size ());
			caption += " file (s)";
		}
		DisplayWindow display (caption.c_str (), TheAppInfo.GetWindow ());
		for (GidList::const_iterator iter = gids.begin (); iter != gids.end (); ++iter)
		{
			GlobalId gid = *iter;
			ItemIterator iterItem = _items.find (gid);
			Assert (iterItem != _items.end ());
			Workspace::ScriptItem const & item = *iterItem->second;
			ScriptCmd const & cmd = item.GetFileCmd ();
			cmd.Dump (display.GetDumpWindow (), _pathFinder);
		}
		// Position dump window
		Win::Placement placement;
		if (_lastOpenDumpWin.IsNull () || !_lastOpenDumpWin.Exists ())
		{
			Registry::UserPreferences preferences;
			preferences.GetListWinPlacement (placement);
		}
		else
		{
			placement.Init (_lastOpenDumpWin);
			Win::Rect rect;
			placement.GetRect (rect);
			rect.ShiftBy (_placementOffset, _placementOffset);
			placement.SetRect (rect);
		}
		if (placement.IsMinimized ())
			placement.SetNormal ();
		display.SetPlacement (placement);
		display.Show ();
		// Remember open dump window
		for (GidList::const_iterator iter = gids.begin (); iter != gids.end (); ++iter)
		{
			GlobalId gid = *iter;
			_openWins [gid] = display;
		}
		_lastOpenDumpWin = display;
	}
}

void ScriptDisplayTable::OpenAll ()
{
	GidList gids;
	std::copy (_files.begin (), _files.end (), std::back_inserter (gids));
	Open (gids);
}

char const * ScriptDisplayTable::GetRootRelativePath (GlobalId gid) const
{
	char const * path = _pathFinder.GetRootRelativePath (gid);
	if (path == 0)
		path = GetFileName (gid);	// For new files return just file name
	return path;
}

char const * ScriptDisplayTable::GetFileName (GlobalId gid) const
{
	ItemIterator iter = _items.find (gid);
	Assert (iter != _items.end ());
	Workspace::ScriptItem const & item = *iter->second;
	if (item.HasEffectiveTarget ())
		return item.GetEffectiveTargetName ().c_str ();
	else
		return item.GetEffectiveSource ().GetName ().c_str ();
}

FileType ScriptDisplayTable::GetFileType (GlobalId gid) const
{
	ItemIterator iter = _items.find (gid);
	Assert (iter != _items.end ());
	Workspace::ScriptItem const & item = *iter->second;
	if (item.HasEffectiveTarget ())
		return item.GetEffectiveTargetType ();
	else
		return item.GetEffectiveSourceType ();
}

bool ScriptDisplayTable::IsFolder (GlobalId gid) const
{
	ItemIterator iter = _items.find (gid);
	Assert (iter != _items.end ());
	Workspace::ScriptItem const & item = *iter->second;
	return item.IsFolder ();
}

FileDisplayTable::ChangeState ScriptDisplayTable::GetFileState (GlobalId gid) const
{
	ItemIterator iter = _items.find (gid);
	Assert (iter != _items.end ());
	Workspace::ScriptItem const & item = *iter->second;
	if (item.HasEffectiveSource ())
	{
		if (item.HasEffectiveTarget ())
		{
			if (item.HasBeenMoved ())
			{
				UniqueName const & source = item.GetEffectiveSource ();
				UniqueName const & target = item.GetEffectiveTarget ();
				if (source.GetParentId () != target.GetParentId ())
					return FileDisplayTable::Moved;
				else
					return FileDisplayTable::Renamed;
			}
			else
			{
				return FileDisplayTable::Changed;
			}
		}
		else
		{
			return FileDisplayTable::Deleted;
		}
	}
	else
	{
		Assert (item.HasEffectiveTarget ());
		return FileDisplayTable::Created;
	}
}

ScriptProps::ScriptProps (Project::Db const & projectDb, PathFinder const & pathFinder)
	: _projectDb (projectDb),
	  _fileTable (new ScriptDisplayTable (pathFinder)),
	  _isFromHistory (true),
	  _isOverdue (false),
	  _isMissingFullSynch (false),
	  _isProjectVerificationRequest (false)
{}

ScriptProps::ScriptProps (Project::Db const & projectDb, PathFinder const & pathFinder, std::string const & path)
	: _projectDb (projectDb),
	  _fileTable (new ScriptDisplayTable (pathFinder)),
	  _isFromHistory (false),
	  _isOverdue (false),
	  _isMissingFullSynch (false),
	  _isProjectVerificationRequest (false)
{
	ScriptReader reader (path);
	std::unique_ptr<ScriptHeader> hdr = reader.RetrieveScriptHeader ();
	hdr->Verify ();
	std::unique_ptr<CommandList> cmdList = reader.RetrieveCommandList ();
	Init (std::move(hdr), std::move(cmdList));
	// Display to whom this script was send
	std::unique_ptr<TransportHeader> txHdr = reader.RetrieveTransportHeader ();
	AddresseeList const & recipients = txHdr->GetRecipients ();
	for (AddresseeList::ConstIterator iter = recipients.begin (); iter != recipients.end (); ++iter)
	{
		Addressee const & recipient = *iter;
		UserIdPack pack (recipient.GetStringUserId ());
		Assert (!pack.IsRandom ());
		AddUserInfo (projectDb, pack.GetUserId ());
	}
}

ScriptProps::ScriptProps (Project::Db const & projectDb, Sidetrack const & sidetrack, GlobalId scriptId)
	: _projectDb (projectDb),
	  _isFromHistory (false),
	  _isOverdue (false),
	  _isMissingFullSynch (false),
	  _requestedScriptId (scriptId),
	  _nextRecipientId (gidInvalid)
{
	Sidetrack::Missing const * missing = sidetrack.GetMissing (scriptId);
	_requestedUnitType = missing->UnitType ();
	if (!missing->IsExhausted ())
	{
		_nextRecipientId = missing->NextRecipientId ();
		_nextRequestTime = missing->NextTime ();
	}
	std::vector<UserId> const & requestRecipients = missing->GetRequestRecipients ();
	for (std::vector<UserId>::const_iterator iter = requestRecipients.begin (); iter != requestRecipients.end (); ++iter)
	{
		UserId userId = *iter;
		if (userId != gidInvalid)
		{
			AddUserInfo (projectDb, userId);
		}
	}
	if (sidetrack.IsMissingChunked (scriptId, _receivedPartCount, _partCount))
	{
		// Missing script is chunked
		Sidetrack::MissingChunk const * missingChunk = dynamic_cast<Sidetrack::MissingChunk const *>(missing);
		_maxChunkSize = missingChunk->GetMaxChunkSize ();
		_isMissingFullSynch = missingChunk->IsFullSynch ();
	}
	else
	{
		// We are missing a complete script
		_partCount = 1;
		_maxChunkSize = 0;
		_receivedPartCount = 0;
	}
}

void ScriptProps::Init (std::unique_ptr<ScriptHeader> hdr, std::unique_ptr<CommandList> cmdList)
{
	_hdr = std::move(hdr);
	if (_isFromHistory)
	{
		// Script retrieved from the history -- display who did not acknowledge this script
		for (GidList::const_iterator iter = _ackList.begin (); iter != _ackList.end (); ++iter)
		{
			UserId userId = *iter;
			AddUserInfo (_projectDb, userId);
		}
	}
	if (_hdr->IsSetChange ())
	{
		ScriptDisplayTable * displayTable = reinterpret_cast<ScriptDisplayTable *>(_fileTable.get ());
		displayTable->Init (std::move(cmdList));
	}
	else if (_hdr->IsMembershipChange ())
	{
		Assert (cmdList->size () == 1);
		CommandList::Sequencer seq (*cmdList);
		if (_hdr->IsAddMember ())
		{
			NewMemberCmd const & cmd = seq.GetAddMemberCmd ();
			MemberInfo const & info = cmd.GetMemberInfo ();
			_updateInfo.reset (new MemberInfo (info));
		}
		else if (_hdr->IsDefectOrRemove ())
		{
			DeleteMemberCmd const & cmd = seq.GetDeleteMemberCmd ();
			MemberInfo const & info = cmd.GetMemberInfo ();
			_updateInfo.reset (new MemberInfo (info));
		}
		else
		{
			Assert (_hdr->IsEditMember ());
			EditMemberCmd const & cmd = seq.GetEditMemberCmd ();
			_currentInfo.reset (new MemberInfo (cmd.GetOldMemberInfo ()));
			_updateInfo.reset (new MemberInfo (cmd.GetNewMemberInfo ()));
		}
	}
	else if (_hdr->IsPureControl ())
	{
		CommandList::Sequencer seq (*cmdList);
		if (_hdr->IsAck ())
		{
			for ( ; !seq.AtEnd (); seq.Advance ())
			{
				CtrlCmd const & cmd = seq.GetCtrlCmd ();
				_acks.push_back (std::make_pair<GlobalId, bool>(cmd.GetScriptId (), (cmd.GetType () == typeMakeReference)));
			}
		}
		else if (_hdr->IsJoinRequest ())
		{
			JoinRequestCmd const & cmd = seq.GetJoinRequestCmd ();
			_updateInfo.reset (new MemberInfo (cmd.GetMemberInfo ()));
		}
		else if (_hdr->IsScriptResendRequest ())
		{
			CtrlCmd const & cmd = seq.GetCtrlCmd ();
			_requestedScriptId = cmd.GetScriptId ();
			if (cmd.GetType () == typeVerificationRequest)
			{
				_isProjectVerificationRequest = true;
				VerificationRequestCmd const & verificationRequestCmd = seq.GetVerificationRequestCmd ();
				_knownDeadMembers = verificationRequestCmd.GetKnownDeadMembers ();
			}
		}
		else if (_hdr->IsFullSynchResendRequest ())
		{
			ResendFullSynchRequestCmd const & cmd = seq.GetFsResendCmd ();
			_requestedScriptId = cmd.GetScriptId ();
		}
	}
	if (_isFromHistory)
	{
		UserId thisUserId = _projectDb.GetMyId ();
		if (thisUserId == _hdr->SenderUserId ())
		{
			// My script
			_isOverdue = !_ackList.empty () 
						&& IsTimeOlderThan (_hdr->GetTimeStamp (), 2 * Week);
		}
	}
}

ScriptProps::UserInfo::UserInfo (UserId userId, MemberDescription const & member)
	: _name (member.GetName ()),
	  _hubId (member.GetHubId ()),
	  _id (ToHexString (userId))
{}

UserId ScriptProps::UserInfo::GetUserId() const
{
	unsigned long result;
	HexStrToUnsigned(_id.c_str(), result);
	return static_cast<UserId>(result);
}

void ScriptProps::AddUserInfo (Project::Db const & projectDb, UserId userId)
{
	if (projectDb.IsProjectMember (userId))
	{
		std::unique_ptr<MemberDescription> member = projectDb.RetrieveMemberDescription (userId);
		Assert (member.get () != 0);
		UserInfo userInfo (userId, *member);
		_users.push_back (userInfo);
	}
	else
	{
		UserInfo userInfo;
		userInfo._hubId = "Unknown";
		userInfo._name = "Unknown";
		userInfo._id = ToHexString (userId);
		_users.push_back (userInfo);
	}
}

char const * ScriptProps::GetCaption () const
{
	if (IsMissing ())
	{
		if (_isMissingFullSynch)
			return "Missing Full Synch Properties";
		else
			return "Missing Script Properties";
	}
	else if (_hdr->IsData ())
		return "Data Script Properties";
	else
		return "Control Script Properties";
}

std::string const & ScriptProps::GetCheckinComment () const
{
	return _hdr->GetComment ();
}

GlobalId ScriptProps::GetScriptId () const
{
	if (IsMissing ())
		return _requestedScriptId;
	else
		return _hdr->ScriptId ();
}

std::unique_ptr<MemberInfo> ScriptProps::RetrieveSenderInfo () const
{
	if (!IsMissing () && IsJoinRequest ())
	{
		// User joining the project
		return std::unique_ptr<MemberInfo> (new MemberInfo (*_updateInfo));
	}
	else
	{
		GlobalId scriptId = GetScriptId ();
		UserId senderId = GlobalIdPack (scriptId).GetUserId ();
		return RetrieveMemberInfo (senderId);
	}
}

std::unique_ptr<MemberInfo> ScriptProps::RetrieveMemberInfo (UserId userId) const
{
	if (_projectDb.IsProjectMember (userId))
		return _projectDb.RetrieveMemberInfo (userId);
	// User is not recorded yet in the project database
	MemberDescription description ("Not recorded yet",	// Name
								   "Unknown",			// Hub's Email Address
								   "",				    // Phone
								   "",					// License
								   "");					// User id
	MemberState state;
	return std::unique_ptr<MemberInfo> (new MemberInfo (userId, state, description));
}

long ScriptProps::GetScriptTimeStamp () const
{
	return _hdr->GetTimeStamp ();
}

std::string ScriptProps::GetProjectName () const
{
	return _hdr->GetProjectName ();
}

unsigned ScriptProps::GetPartCount () const
{
	if (IsMissing ())
		return _partCount;
	else
		return _hdr->GetPartCount ();
}

unsigned ScriptProps::GetPartNumber () const
{
	Assert (!IsMissing ());
	return _hdr->GetPartNumber ();
}

unsigned ScriptProps::GetMaxChunkSize () const
{
	if (IsMissing ())
		return _maxChunkSize;
	else
		return _hdr->GetMaxChunkSize ();
}

bool ScriptProps::IsSetChange () const
{
	return _hdr->IsSetChange ();
}

bool ScriptProps::IsCtrl () const
{
	return _hdr->IsControl ();
}

bool ScriptProps::IsJoinRequest () const
{
	return _hdr->IsJoinRequest ();
}

bool ScriptProps::IsScriptResendRequest () const
{
	return _hdr->IsScriptResendRequest ();
}

bool ScriptProps::IsFullSynchResendRequest () const
{
	if (IsMissing ())
		return _isMissingFullSynch;
	else
		return _hdr->IsFullSynchResendRequest ();
}

bool ScriptProps::IsFullSynch () const
{
	return _hdr->IsFullSynch ();
}

bool ScriptProps::IsPackage () const
{
	return _hdr->IsPackage ();
}

bool ScriptProps::IsAwaitingFinalAck () const
{
	if (IsFromHistory () && _hdr->IsSetChange () && _users.size () == 1)
	{
		GlobalIdPack sender (GetScriptId ());
		return ToHexString (sender.GetUserId ()) == _users [0]._id;
	}
	return false;
}

bool ScriptProps::IsMembershipUpdate () const
{
	return _hdr->IsMembershipChange ();
}

bool ScriptProps::IsAddMember () const
{
	return _hdr->IsAddMember ();
}

bool ScriptProps::IsDefectOrRemove () const
{
	return _hdr->IsDefectOrRemove ();
}

bool ScriptProps::IsAck () const
{
	return _hdr->IsAck ();
}

ScriptProps::LineageSequencer::LineageSequencer (ScriptProps const & props)
	: _lineage (props._hdr->GetLineage ()),
	  _sequencer (_lineage),
	  _projectDb (props._projectDb)
{
	Init ();
}

void ScriptProps::LineageSequencer::Advance ()
{
	_sequencer.Advance ();
	Init ();
}

void ScriptProps::LineageSequencer::Init ()
{
	if (AtEnd ())
		return;

    GlobalIdPack pack (_sequencer.GetScriptId ());
	std::unique_ptr<MemberDescription> sender = _projectDb.RetrieveMemberDescription (pack.GetUserId ());
	_id = pack.ToString ();
	_sender = MemberNameTag (sender->GetName (), pack.GetUserId ());
    if (pack == _lineage.GetReferenceId ())
		_sender += " -- reference script";
}
