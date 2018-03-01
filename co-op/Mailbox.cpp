//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "Mailbox.h"
#include "ScriptIo.h"
#include "ScriptHeader.h"
#include "ScriptTrailer.h"
#include "ScriptCommandList.h"
#include "ScriptList.h"
#include "CmdExec.h"
#include "PathFind.h"
#include "ProjectDb.h"
#include "History.h"
#include "OutputSink.h"
#include "TransportHeader.h"
#include "DispatcherScript.h"
#include "FileList.h"
#include "Mailer.h"
#include "MailboxScriptState.h"
#include "HistoryScriptState.h"
#include "AckBox.h"
#include "VersionInfo.h"
#include "AppInfo.h"
#include "CheckoutNotifications.h"

#include <Ctrl/ProgressMeter.h>
#include <File/Dir.h>
#include <File/File.h>
#include <Ex/WinEx.h>

using namespace Mailbox;

Db::Db (Project::Db & projectDb, History::Db & history, Sidetrack & sidetrack) 
	: _cache (projectDb),
	  _projectDb (projectDb),
	  _history (history),
	  _sidetrack (sidetrack)
{
	_scriptPatterns.push_back ("*.snc");
	_scriptPatterns.push_back ("*.cnk");
}

void Db::InitPaths (PathFinder & pathFinder)
{
	_inboxPath.Change (pathFinder.InBoxDir ().GetDir ());
	_cache.Invalidate ();
}

void Db::Dump (std::ostream  & out) const
{
	if (!_inboxScripts.empty ())
	{
		out << "===Scripts files recorded in the project's inbox:" << std::endl << std::endl;
		for (std::vector<Mailbox::ScriptInfo>::const_iterator iter = _inboxScripts.begin ();
			 iter != _inboxScripts.end ();
			 ++iter)
		{
			ScriptInfo const & info = *iter;
			try
			{
				FileDeserializer in (info.GetScriptPath ());
				TransportHeader txHdr (in);
				out << txHdr << std::endl;
			}
			catch ( ... )
			{
				out << "Cannot read script transport header" << std::endl;
			}
			out << std::endl;
			out << info << std::endl << std::endl;
		}
	}

	FileMultiSeq files (_inboxPath, _scriptPatterns);
	if (!files.AtEnd ())
	{
		out << "===Scripts files present in the folder '" << _inboxPath.GetDir () << "':" << std::endl << std::endl;
		while (!files.AtEnd ())
		{
			out << "*" << files.GetName () << std::endl;
			files.Advance ();
		}
	}

	_history.DumpFileChanges (out);
}

class IsFromFuture : public std::unary_function<ScriptInfo const &, bool>
{
public:
	bool operator () (ScriptInfo const & info) const
	{
		return info.IsFromFuture ();
	}
};

bool Db::HasScriptsFromFuture () const
{
	std::vector<ScriptInfo>::const_iterator iter =
		std::find_if (_inboxScripts.begin (), _inboxScripts.end (), IsFromFuture ());
	return iter != _inboxScripts.end ();
}

bool Db::HasExecutableJoinRequest () const
{
	return _projectDb.IsProjectAdmin () && (GetJoinRequest () != 0);
}

// Table interface

void Db::QueryUniqueIds (Restriction const & restrict, GidList & ids) const
{
	dbg << "Mailbox Query Unique Ids" << std::endl;
	// Mailbox view shows the following script kinds:
	// - from the future, wrongly addressed or corrupted
	for (std::vector<ScriptInfo>::const_iterator iter = _inboxScripts.begin ();
		 iter != _inboxScripts.end ();
		 ++iter)
	{
		ScriptInfo const & info = *iter;
		dbg << "    " << GlobalIdPack (info.GetScriptId ()) << std::endl;
		ids.push_back (info.GetScriptId ());
	}
	// - missing disconnected set scripts
	_history.GetDisconnectedMissingSetScripts (ids);
	// - scripts unpacked but not executed yet
	_history.GetUnpackedScripts (ids);
}

class IsEqualScriptId : public std::unary_function<ScriptInfo const &, bool>
{
public:
	IsEqualScriptId (GlobalId id)
		: _id (id)
	{}
	bool operator () (ScriptInfo const & info) const
	{
		return info.GetScriptId () == _id;
	}
private:
	GlobalId	_id;
};

class IsJoinRequest : public std::unary_function<ScriptInfo const &, bool>
{
public:
	bool operator () (ScriptInfo const & info) const
	{
		return info.IsJoinRequest ();
	}
};

std::string Db::GetStringField (Column col, GlobalId gid) const
{
	if (col == colFrom)
	{
		// Retrieve sender name -- gid is global user id
		GlobalIdPack pack (gid, 0);
		if (pack.IsFromJoiningUser ())
			return "Member joining the project";

		if (_history.IsFullSyncExecuted ())
		{
			MemberDescription const * sender = _cache.GetMemberDescription (gid);
			return sender->GetName ();
		}
		else
			return "Administrator";
	}

	// Retrieve script info -- gid is global script id
	std::vector<ScriptInfo>::const_iterator iter =
		std::find_if (_inboxScripts.begin (), _inboxScripts.end (), IsEqualScriptId (gid));
	if (iter == _inboxScripts.end ())
	{
		// Try the sidetrack first
		if (_sidetrack.IsMissing (gid))
		{
			if (col == colStateName)
			{
				// Have we executed the full sync script?
				if (_history.IsFullSyncExecuted ())
					return "Missing";
				else
					return "Patience! More parts on the way";
			}

			if (col == colTimeStamp)
				return "";

			Assert (col == colName || col == colVersion);
			if (_sidetrack.NeedsUpdate (gid) && _sidetrack.NextRecipientId (gid) == gidInvalid)
				return "Resend requests exhausted";

			std::ostringstream missingInfo; // "Missing N of M script chunks. " "Missing. "
			unsigned chunksReceived, chunksTotal;
			bool isChunked = _sidetrack.IsMissingChunked (gid, chunksReceived, chunksTotal);
			if (isChunked)
			{
				missingInfo << "Missing " << chunksTotal - chunksReceived << " of " << chunksTotal 
					<< " script chunks. ";
			}
			else
			{
				missingInfo << "Missing. ";
			}

			std::ostringstream str;
			if (_history.IsFullSyncExecuted ())
			{
				str << missingInfo.str ();
				str << "Waiting for a re-send from ";
				UserId uid = _sidetrack.SentTo (gid);
				if (uid == gidInvalid)
					return "Missing. There is nobody to ask for a re-send";
			
				MemberDescription const * sentTo = _cache.GetMemberDescription (uid);
				MemberNameTag nameTag (sentTo->GetName (), uid);
				str << nameTag;
			}
			else
			{
				str << "Waiting for the Full Sync script. ";
				if (isChunked)
					str << missingInfo.str ();
				str << "A re-send from project Administrator has been requested";
			}
			return str.str ();
		}

		// Unpacked script -- look in the history
		if (col == colStateName && _history.IsNext (gid))
		{
			// If we have the join request in the mailbox folder then give it priority
			// over the script recorded in the history if I'm the project administrator
			if (_projectDb.IsProjectAdmin ())
			{
				std::vector<ScriptInfo>::const_iterator iter =
					std::find_if (_inboxScripts.begin (), _inboxScripts.end (), IsJoinRequest ());
				if (iter == _inboxScripts.end ())
					return "<-Next";
			}
			else
			{
				return "<-Next";
			}
		}
		
		return _history.GetStringField (col, gid);
	}
	else
	{
		// Script in the inbox folder
		ScriptInfo const & info = *iter;
		if (col == colName)
			return info.Caption ();
		if (col == colVersion)
			return _history.GetStringField (col, gid);
		if (col == colTimeStamp)
			return info.GetTimeStamp ();

		Assert (col == colStateName);

		if (info.IsCorrupted ())
		{
			std::string const & errorMessage = info.GetErrorMsg ();
			if (!errorMessage.empty ())
				return errorMessage;
			else if (info.IsSetChange ())
				return "Corrupted--ask the sender to re-send it manually";
			else
				return "Corrupted--ignored";
		}
		else if (info.IsIllegalDuplicate ())
		{
			return "Invalid duplicate -- ignored";
		}
		else if (info.IsForThisProject ())
		{
			if (info.IsJoinRequest ())
			{
				if (_projectDb.IsProjectAdmin ())
					return "<-Next";
				else
					return "Cannot forward to administrator";
			}
			else
			{
				return "Awaiting its turn";
			}
		}
		return "Not for this project";
	}
}

class ScriptStateTranslator
{
public:
	ScriptStateTranslator (History::ScriptState historyState)
	{
		// Clear history specific bits
		historyState.SetCurrent (false);
		historyState.SetProjectCreationMarker (false);
		historyState.SetInteresting (false);
		historyState.SetConfirmed (false);
		_value = historyState.GetValue ();
	}

	Mailbox::ScriptState GetMailboxScriptState () const { return Mailbox::ScriptState (_value); }

private:
	unsigned long	_value;
};

unsigned long Db::GetNumericField (Column col, GlobalId gid) const
{
	Assert (col == colState);
	std::vector<ScriptInfo>::const_iterator iter =
		std::find_if (_inboxScripts.begin (), _inboxScripts.end (), IsEqualScriptId (gid));
	if (iter == _inboxScripts.end ())
	{
		// Unpacked script in the history
		// Try the sidetrack first
		if (_sidetrack.IsMissing (gid))
		{
			Mailbox::ScriptState state;
			state.SetMissing (true);
			state.SetForThisProject (true);
			return state.GetValue ();
		}

		History::ScriptState historyState (_history.GetNumericField (col, gid));
		ScriptStateTranslator translator (historyState);
		Mailbox::ScriptState mailboxState = translator.GetMailboxScriptState ();
		// Set mailbox specific bits
		mailboxState.SetForThisProject (true);
		if (_history.IsNext (gid))
		{
			// Unpacked script is marked as NEXT in the history.
			// Check if we have join request in the mailbox folder and
			// give it priority.
			if (_projectDb.IsProjectAdmin ())
				mailboxState.SetNext (GetJoinRequest () == 0);
			else
				mailboxState.SetNext (true);
		}
		return mailboxState.GetValue ();
	}
	else
	{
		// Script present in the local inbox folder
		ScriptInfo const & info = *iter;
		Mailbox::ScriptState mailboxState (info.GetScriptState ().GetValue ());
		if (mailboxState.IsForThisProject () && info.IsJoinRequest ())
		{
			// Mark the join request as NEXT only if I'm the project administator
			mailboxState.SetNext (_projectDb.IsProjectAdmin ());
		}
		return mailboxState.GetValue ();
	}
}

GlobalId Db::GetIdField (Column col, GlobalId gid) const
{
	return gidInvalid;
}

std::string Db::GetStringField (Column col, UniqueName const & uname) const
{
	return std::string ();
}

GlobalId Db::GetIdField (Column col, UniqueName const & uname) const
{
	return gidInvalid;
}

std::string Db::GetCaption (Restriction const & restrict) const
{
	std::string caption = _history.RetrieveNextScriptCaption ();
	if (caption.empty ())
	{
		// There are no scripts ready for execution in the history.
		// Check if there is join request sitting in the mailbox
		for (std::vector<ScriptInfo>::const_iterator iter = _inboxScripts.begin ();
			 iter != _inboxScripts.end ();
			 ++iter)
		{
			ScriptInfo const & info = *iter;
			if (iter->GetScriptState ().IsJoinRequest ())
			{
				caption = info.Caption ();
				break;
			}
		}
	}
	if (caption.empty ())
		caption.assign ("No scripts to execute");
	return caption;
}

bool Db::ScriptFilesPresent () const
{
	FileMultiSeq inBoxScripts (_inboxPath, _scriptPatterns);
	return !inBoxScripts.AtEnd ();
}

bool Db::IsScriptFilePresent (GlobalId scriptId) const
{
	std::vector<ScriptInfo>::const_iterator iter =
		std::find_if (_inboxScripts.begin (), _inboxScripts.end (), IsEqualScriptId (scriptId));
	return iter != _inboxScripts.end ();
}

bool Db::IsJoinRequestScript (GlobalId scriptId) const
{
	std::vector<ScriptInfo>::const_iterator iter =
		std::find_if (_inboxScripts.begin (), _inboxScripts.end (), IsEqualScriptId (scriptId));
	if (iter != _inboxScripts.end ())
	{
		ScriptInfo const & scriptInfo = *iter;
		return scriptInfo.IsJoinRequest ();
	}
	return false;
}

namespace Mailbox
{
	class ScriptFile
	{
	public:
		ScriptFile (Mailbox::ScriptState state)
			: _state (state)
		{}

		ScriptFile ()
		{
			_state.SetForThisProject (true);	// Assume the script is for this project
		}

		void Open (FilePath const & inboxPath, std::string const & fileName, std::string const & thisProjectName)
		{
			_fileName = fileName;
			_path = inboxPath.GetFilePath (_fileName);
			_reader.SetInputPath (_path);
			if (File::Exists (_path.c_str ()))
			{
				FileDeserializer in (_path);
				_hdr = _reader.RetrieveScriptHeader (in);
				_hdr->Verify ();
				_trailer = _reader.RetrieveScriptTrailer (in);
				_state.SetForThisProject (IsFileNameEqual (thisProjectName, _hdr->GetProjectName ()));
				_state.SetJoinRequest (_hdr->IsJoinRequest ());
			}
		}

		void MarkFromFuture () { _state.SetFromFuture (true); }
		void MarkIllegalDuplicate () { _state.SetIllegalDuplicate (true); }
		void MarkCorrupted () { _state.SetCorrupted (true); }

		bool IsValidHeader () const { return _hdr.get () != 0; }

		GlobalId GetId () const { return _hdr->ScriptId (); }
		GlobalId GetModifiedUnitId () const { return _hdr->GetModifiedUnitId (); }
		unsigned GetPartCount () const { return _hdr->GetPartCount (); }
		ScriptHeader const & GetHdr () const { return *_hdr; }
		ScriptTrailer const & GetTrailer () const { return *_trailer; }
		GlobalId GetReferenceId () const
		{
			return _hdr->GetLineage ().GetReferenceId ();
		}
		ScriptInfo GetInfo () const
		{
			if (IsValidHeader ())
				return ScriptInfo (_state, _path, _hdr.get ());
			else
				return ScriptInfo (_state, _path);
		}
		std::string const & GetFileName () const { return _fileName; }
		std::string const & GetPath () const { return _path; }

		std::unique_ptr<CommandList> RetrieveCmdList ()
		{
			return _reader.RetrieveCommandList ();
		}

		std::unique_ptr<CheckOut::List> RetrieveCheckoutNotification ()
		{
			FileDeserializer in (_path);
			std::unique_ptr<CheckOut::List> notification =
				_reader.RetrieveCheckoutNotification (in);
			Assert (IsValidHeader ());
			GlobalIdPack pack (_hdr->ScriptId ());
			notification->SetSenderId (pack.GetUserId ());
			return notification;
		}
		bool IsForThisProject () const { return _state.IsForThisProject (); }
		bool IsControl () const { return _hdr->IsControl (); }
		bool IsData () const { return _hdr->IsData (); }
		bool IsJoinRequest () const { return _state.IsJoinRequest (); }
		bool IsMembershipChange () const { return _hdr->IsMembershipChange (); }
		bool IsSetChange () const { return _hdr->IsSetChange (); }
		bool IsPackage () const { return _hdr->IsPackage (); }
		bool IsVerificationPackage () const { return _hdr->IsVerificationPackage (); }
		bool IsChunk () const { return _hdr->IsChunk (); }
		bool IsFullSynch () const { return _hdr->IsFullSynch (); }
		bool IsCorrupted () const { return _state.IsCorrupted (); }
		bool HasTrailer () const { return !_trailer->empty (); }

	private:
		ScriptState						_state;
		std::string						_fileName;
		std::string						_path;
		ScriptReader					_reader;
		std::unique_ptr<ScriptHeader>		_hdr;
		std::unique_ptr<ScriptTrailer>	_trailer;
	};
}

void Db::RememberScript (std::string const & fileName, 
						 Mailbox::ScriptState state, 
						 std::string const & errorMsg)
{
	ScriptFile script (state);
	try
	{
		script.Open (_inboxPath, fileName, _projectDb.ProjectName ());
		if (!script.IsValidHeader ())
			script.MarkCorrupted (); // Couldn't open script file
	}
	catch (Win::Exception e)
	{
		if (!e.IsSharingViolation ())
		{
			script.MarkCorrupted ();
		}
	}
	catch ( ... )
	{
		// Exception during script processing -- probably corrupted script -- ignore it
		script.MarkCorrupted ();
		Win::ClearError ();
	}
	ScriptInfo scriptInfo = script.GetInfo ();
	scriptInfo.SetErrorMsg (errorMsg);
	_inboxScripts.push_back (scriptInfo);
}

// Returns false when corrupted scripts detected in the inbox folder
bool Db::XUnpackScripts (Mailbox::Agent & agent, CorruptedScriptMap & corruptedScripts, Progress::Meter * meter)
{
	dbg << "--> Mailbox::XUnpackScripts" << std::endl;
	_inboxScripts.clear ();
	// Count script files skipping corrupted ones
	std::vector<std::string> scriptFileNames;
	for (FileMultiSeq inBoxScripts (_inboxPath, _scriptPatterns);
		 !inBoxScripts.AtEnd ();
		 inBoxScripts.Advance ())
	{
		std::string scriptName = inBoxScripts.GetName ();
		if (corruptedScripts.find (scriptName) == corruptedScripts.end ())
			scriptFileNames.push_back (scriptName);
	}

	if (scriptFileNames.size () == 0)
		return true;

	std::string thisProjectName (_projectDb.XProjectName ());
	meter->SetActivity ("Unpacking Incoming Scripts");
	meter->SetRange (0, scriptFileNames.size (), 1);
	// Unpack scripts from the in-box
	for (std::vector<std::string>::const_iterator iter = scriptFileNames.begin ();
		 iter != scriptFileNames.end ();
		 ++iter)
	{
		bool isCorrupted = true;
		std::string const & fileName = *iter;
		dbg << "     Unpacking script: " << fileName << std::endl;
		std::string errorReport;
		ScriptFile script;
		try
		{
			script.Open (_inboxPath, fileName, thisProjectName);
			if (!script.IsValidHeader ())
				continue;	// Couldn't open script file

			if (XCanUnpackScript (script, agent))
			{
				agent.RememberCheckoutNotification (script.RetrieveCheckoutNotification ());
				if (!XUnpackScript (script, agent))
				{
					// Illegal script duplicate found -- ignore it
					script.MarkIllegalDuplicate ();
					_inboxScripts.push_back (script.GetInfo ());
				}
			}
			isCorrupted = false;
		}
		catch (Win::Exception e)
		{
			errorReport = Out::Sink::FormatExceptionMsg (e);
			dbg << "     Exception: " << errorReport << std::endl;
			if (e.IsSharingViolation ())
				isCorrupted = false;
		}
		catch (...)
		{
			// Exception during script processing -- probably corrupted script -- ignore it
			dbg << "    Unknown exception" << std::endl;
			Win::ClearError ();
		}

		if (isCorrupted)
		{
			corruptedScripts [fileName] = errorReport;
			agent.LogCorruptedScript (script.GetInfo ());
			return false;
		}
		meter->StepIt ();
	}
	meter->Close ();
	// Remove scripts already present from the list of disconnected scripts
	for (std::vector<ScriptInfo>::const_iterator it = _inboxScripts.begin ();
		it != _inboxScripts.end (); ++it)
	{
		ScriptInfo info = *it;
		if (info.IsFromFuture ())
		{
			_history.XRemoveDisconnectedScript (info.GetScriptId (), 
												info.GetUnitType ());
			if (info.GetUnitType () == Unit::Set)
				_projectDb.XUpdateSender (info.GetScriptId ());
		}
	}
	dbg << "<-- Mailbox::XUnpackScripts" << std::endl;
	return true;
}

bool Db::XCanUnpackScript (ScriptFile & script, Agent & agent)
{
	if (!script.IsForThisProject () ||
		(!_history.IsFullSyncExecuted () && !script.IsFullSynch ()))
	{
		// Script not for this project or
		// we are awaiting the full sync script and the incoming script is not the full sync
		// -- hold the incoming script in the inbox
		_inboxScripts.push_back (script.GetInfo ());
		return false;	// Process next script file
	}

	if (script.IsJoinRequest ())
	{
		if (_projectDb.XIsProjectAdmin ())
		{
			// This is project administrator enlistment -- leave join request in the
			// inbox, so user can unpack it.
			_inboxScripts.push_back (script.GetInfo ());
		}
		else
		{
			// This is not project administrator enlistment -- treat join request as
			// any control script, so it will be processed immediately.
			agent.RememberCtrlScript (script.GetInfo ());
		}
		return false;	// Process next script file
	}

	GlobalId scriptId = script.GetId ();
	if (scriptId == gidInvalid ||
		(!script.IsMembershipChange () && _projectDb.XIsPrehistoric (scriptId) == Tri::Yes))
	{
		// Script created before we have joined the project or
		// has invalid global id -- delete it from the inbox
		agent.DeleteScriptFile (script.GetPath ());
		return false;	// Process next script file
	}

	Assert (script.IsForThisProject () &&
		    (_history.IsFullSyncExecuted () || agent.GetThisUserAgent ().IsMyStateValid () || script.IsFullSynch ()));
	return true;
}

// Returns true when script unpacked successfully
bool Db::XUnpackScript (ScriptFile & script, Agent & agent)
{
	Assert (script.IsForThisProject () && script.GetId () != gidInvalid &&
		    (_history.IsFullSyncExecuted () || agent.GetThisUserAgent ().IsMyStateValid () || script.IsFullSynch ()));

	agent.SetCurrentScript (script.GetId ());
	bool scriptUnpackedSuccessfully = true;

	if (script.IsChunk ())
	{
		XUnpackChunk (script, agent);
	}
	else if (script.IsFullSynch ())
	{
		// Full sync package
		if (!_history.IsFullSyncExecuted () && !_history.XIsFullSyncUnpacked ())
		{
			FileDeserializer in (script.GetPath ());
			ScriptList fullSynch (in);
			XUnpackFullSynch (fullSynch, GlobalIdPack (script.GetId ()).GetUserId (), agent);
			// Full synch is the last script we've seen from the admin
			_projectDb.XUpdateSender (script.GetId ());
		}
		agent.DeleteScriptFile (script.GetPath ());
	}
	else
	{
		History::Status scriptStatus = _history.XProcessLineages (
			script.GetHdr (), 
			agent, 
			!script.IsPackage ());// Process main lineage if not package

		if (scriptStatus == History::Connected)
		{
			scriptUnpackedSuccessfully = XInsertScript (script, agent);
		}
		else if (scriptStatus == History::Disconnected)
		{
			// At least one script lineage doesn't connect with our history.
			if (script.GetHdr ().IsDefectOrRemove ())
			{
				// We have defect script from the future.
				// Force its execution right now and delete the script from the in-box.
				agent.RememberExecuteOnlyScript (script.GetInfo ());
				// Remove this script from the missing disconnected list
				ScriptHeader const & hdr = script.GetHdr ();
				_history.XRemoveDisconnectedScript (hdr.ScriptId (), hdr.GetUnitType ());
				if (hdr.GetUnitType () == Unit::Set)
					_projectDb.XUpdateSender (hdr.ScriptId ());
			}
			else
			{
				// We cannot unpack script and it stays in the in-box folder.
				script.MarkFromFuture ();
				_inboxScripts.push_back (script.GetInfo ());
				agent.SetPendingWork (true);
			}
		}
		else
		{
			// Script main lineage is prehistoric
			Assert (scriptStatus == History::Prehistoric);
			agent.DeletePrehistoricScript (script.GetId (), script.GetPath ());
		}
	}
	_history.XUpdateNextMarker ();
	return scriptUnpackedSuccessfully;
}

void Db::XUnpackChunk (ScriptFile & script, Agent & agent)
{
	dbg << "--> Mailbox::XUnpackChunk" << std::endl;
	Assert (script.IsChunk ());
	if (script.IsFullSynch () &&
		(_history.IsFullSyncExecuted () || _history.XIsFullSyncUnpacked ()))
	{
		agent.DeleteScriptFile (script.GetPath ());
		return;
	}

	unsigned numStored = _sidetrack.XAddChunk (script.GetHdr (), script.GetFileName ().c_str ());
	agent.DeleteScriptFile (script.GetPath ());
	if (numStored == 1) // first chunk
	{
		if (script.HasTrailer ())
			XExecuteScriptTrailer (script.GetTrailer (), agent);

		if (script.IsFullSynch ())
		{
			dbg << "     Full sync chunk received" << std::endl;
			if (!_history.IsFullSyncExecuted ())
			{
				GlobalId inventoryId = script.GetReferenceId ();
				dbg << "     History NOT initialized: inventory id = " << GlobalIdPack (inventoryId) << std::endl;
				_history.XAddMissingInventoryMarker (inventoryId);
			}
		}
		else
		{
			if (!script.IsPackage ())
				agent.SetKnownMissingScript (script.GetId (), script.GetModifiedUnitId ());
			History::Status scriptStatus = _history.XProcessLineages (script.GetHdr (), agent, !script.IsPackage ());
			if (scriptStatus == History::Prehistoric)
			{
				// Script chunk main lineage is prehistoric
				agent.DeletePrehistoricScript (script.GetId (), script.GetPath ());
				_sidetrack.XRemoveMissingScript (script.GetId ());
			}
		}
	}
	else if (numStored == script.GetPartCount ()) // last chunk
	{
		dbg << "     Last script chunk received" << std::endl;
		// Sidetrack deposits the whole script in the inbox. Force another run to unpack it
		agent.SetScriptUnpacked (true);
		agent.SetPendingWork (true);
	}
	dbg << "<-- Mailbox::XUnpackChunk" << std::endl;
}

// Returns true when script successfully inserted into the history or control script list in the mailbox
bool Db::XInsertScript (ScriptFile & script, Agent & agent)
{
	Assert (!script.IsChunk () && !script.IsFullSynch ());
	if (script.IsSetChange ())
		_projectDb.XUpdateSender (script.GetId ());
	if (script.IsControl ())
	{
		// Control script
		if (script.IsPackage ())
		{
			FileDeserializer in (script.GetPath ());
			ScriptList package (in);
			if (!XUnpackPackage (package, agent))
				return false;	// Package not inserted into the history or control script list
			
			if (script.IsVerificationPackage ())
			{
				// Remember to remove project verification marker
				agent.SetDeleteProjectVerificationMarker (true);
			}

			agent.DeleteScriptFile (script.GetPath ());
		}
		else
		{
			// Control scripts are not unpacked -- they are executed
			// directly from the inbox
			agent.RememberCtrlScript (script.GetInfo ());
		}
		return true;
	}

	// Data script
	Assert (script.IsData ());
	InsertDisposition disposition = XGetInsertDisposition (script);
	std::unique_ptr<CommandList> cmdList = script.RetrieveCmdList ();

	if (disposition == DontKnow)
	{
		// Processing the script header didn't tell us if the script should be
		// inserted, executed only or ignored -- inspect script command list.
		Assert (script.GetHdr ().IsFromVersion40 () && script.GetHdr ().IsVersion40EmergencyAdminElection () && cmdList->size () == 1);
		Assert (!_projectDb.XGetMemberState (GlobalIdPack (script.GetHdr ().ScriptId ()).GetUserId ()).IsVerified ());
		CommandList::Sequencer seq (*cmdList);
		EditMemberCmd const & cmd = seq.GetEditMemberCmd ();
		UserId oldMemberId = cmd.GetOldMemberInfo ().Id ();
		UserId newMemberId = cmd.GetNewMemberInfo ().Id ();
		if (_projectDb.XIsProjectMember(oldMemberId) && _projectDb.XIsProjectMember(newMemberId))
		{
			MemberState editedUser1State = _projectDb.XGetMemberState (oldMemberId);
			MemberState editedUser2State = _projectDb.XGetMemberState (newMemberId);
			if (editedUser1State.IsVerified () || editedUser2State.IsVerified ())
				disposition = Ignore;
			else
				disposition = ExecuteOnly;
		}
		else // editing unknown project member
		{
			disposition = Ignore;
		}
	}
	Assert (disposition != DontKnow);

	if (disposition == Insert)
	{
		if (_history.XInsertIncomingScript (script.GetHdr (),
											*cmdList,
											agent.GetAckBox ()))
		{
			// Script inserted into the history
			// Scan script command list and  remember new folders
			for (CommandList::Sequencer seq (*cmdList); !seq.AtEnd (); seq.Advance ())
			{
				ScriptCmd const & cmd = seq.GetCmd ();
				if (cmd.GetType () == typeNewFolder)
				{
					FileCmd const & fileCmd = seq.GetFileCmd ();
					FileData const & fd = fileCmd.GetFileData ();
					agent.RememberNewFolder (fd.GetGlobalId (), fd.GetUniqueName ());
				}
			}
			agent.DeleteScriptFile (script.GetPath ());
			agent.SetScriptUnpacked (true);
			return true;
		}
	}
	else
	{
		Assert (disposition == Ignore || disposition == ExecuteOnly);
		// Remove this script from the missing disconnected list
		ScriptHeader const & hdr = script.GetHdr ();
		_history.XRemoveDisconnectedScript (hdr.ScriptId (), hdr.GetUnitType ());
		if (hdr.GetUnitType () == Unit::Set)
			_projectDb.XUpdateSender (hdr.ScriptId ());
		if (disposition == Ignore)
			agent.DeleteScriptFile (script.GetPath ());
		else
			agent.RememberExecuteOnlyScript (script.GetInfo ());
	}
	return false;	// Script not inserted into the history
}

Db::InsertDisposition Db::XGetInsertDisposition (ScriptFile const & script) const
{
	if (!script.IsMembershipChange ())
		return Insert;

	// Membership update script
	ScriptHeader const & hdr = script.GetHdr ();
	if (!_projectDb.XIsProjectMember (hdr.GetModifiedUnitId ()))
	{
		// We don't know changed project member
		if (hdr.IsFromVersion40 ())
			return ExecuteOnly;	// Execute but don't record in the history
		else
			return Insert;
	}

	UserId senderId = GlobalIdPack (hdr.ScriptId ()).GetUserId ();
	if (!_projectDb.XIsProjectMember (senderId))
	{
		// We don't know the script sender
		if (hdr.IsFromVersion40 ())
			return ExecuteOnly;	// Execute but don't record in the history
		else
			return Insert;
	}

	if (hdr.IsFromVersion40 () && hdr.IsVersion40EmergencyAdminElection ())
		return DontKnow;

	Assert (_projectDb.XIsProjectMember (hdr.GetModifiedUnitId ()) &&
			_projectDb.XIsProjectMember (senderId));
	InsertDisposition disposition = Insert;
	MemberState editedUserState = _projectDb.XGetMemberState (hdr.GetModifiedUnitId ());
	MemberState senderState = _projectDb.XGetMemberState (senderId);
	if (senderState.IsVerified ())
	{
		// Version 4.5 sender
		if (hdr.IsFromVersion40 ())
		{
			// Sender is verified but script is in version 4.2 format
			if (editedUserState.IsVerified ())
			{
				// Version 4.2 script changes data of the version 4.5 user -- ignore update
				disposition = Ignore;
			}
			else
			{
				// Version 4.2 script changes unverified user -- insert virtually
				disposition = ExecuteOnly;
			}
		}
		else
		{
			// Verified user changes some user data -- insert virtually when changed user in unverified
			if (!editedUserState.IsVerified ())
				disposition = ExecuteOnly;
		}
	}
	else
	{
		// Version 4.2 sender
		if (hdr.IsFromVersion40 ())
		{
			// Version 4.2 script format
			if (editedUserState.IsVerified ())
			{
				// Version 4.2 user wants to change data of the version 4.5 user -- ignore update
				disposition = Ignore;
			}
			else
			{
				// Version 4.2 user changes unverified user -- insert virtually
				disposition = ExecuteOnly;
			}
		}
		else
		{
			// Sender is unverified but script is in version 4.5 format
			// Can be conversion membership update or any other script send after conversion
			// by the user itself or project administrator
			Lineage unitLineage;
			_history.XGetUnitLineage (Unit::Member, hdr.GetModifiedUnitId (), unitLineage);
			if (unitLineage.Count () == 0 && !hdr.IsAddMember ())
				disposition = ExecuteOnly;
		}
	}
	return disposition;
}

ScriptInfo const * Db::GetJoinRequest () const
{
	std::vector<ScriptInfo>::const_iterator iter =
		std::find_if (_inboxScripts.begin (), _inboxScripts.end (), IsJoinRequest ());
	if (iter != _inboxScripts.end ())
		return &(*iter);
	else
		return 0;
}

void Db::XExecuteScriptTrailer (ScriptTrailer const & trailer, Mailbox::Agent & agent)
{
	CommandList const & trailerCmds = trailer.GetCmdList ();
	Assert (trailerCmds.size () != 0);
	XExecuteScriptCmds (trailerCmds, agent);
	agent.XRefreshUserData (_projectDb);
}

void Db::XExecuteScriptCmds (CommandList const & cmds, Mailbox::Agent & agent)
{
	for (CommandList::Sequencer seq (cmds); !seq.AtEnd (); seq.Advance ())
	{
		// We expect membership update commands
		MemberCmd const & memberCmd = seq.GetMemberCmd ();
		std::unique_ptr<CmdMemberExec> exec = memberCmd.CreateExec (_projectDb);
		exec->Do (agent.GetThisUserAgent ());
	}
}

void Db::XUnpackFullSynch (ScriptList const & fullSynch, GlobalId fullSynchSenderId, Mailbox::Agent & agent)
{
	dbg << "--> Mailbox::XUnpackFullSynch" << std::endl;
	Assert (!_history.IsFullSyncExecuted ());
	Assert (!_history.XIsFullSyncUnpacked ());
	ScriptList::Sequencer seq (fullSynch);
	Assert (!seq.AtEnd ());
	// When processing the full sync package acknowledge only the first
	// membership update script, which adds joining user to the project membership
	ScriptHeader const & hdr = seq.GetHeader ();
	Assert (hdr.IsAddMember ());
	Unit::ScriptId firstScriptId (hdr.ScriptId (), hdr.GetUnitType ());
	for ( ; !seq.AtEnd (); seq.Advance ())
	{
		ScriptHeader const & hdr = seq.GetHeader ();
		dbg << "     Script header: " << hdr << std::endl;
		CommandList const & cmdList = seq.GetCmdList ();
		if (hdr.IsSetChange ())
		{
			// Set change scripts are appended to the set tree, because there cannot be script conflicts
			_history.XPushBackScript (hdr, cmdList);
		}
		else
		{
			Assert (hdr.IsMembershipChange ());
			// Membership update scripts are inserted into the membership tree, 
			// because there can be script conflicts
			Assert (!hdr.IsPackage ());
			// Process main lineage
			History::Status status = _history.XProcessLineages (hdr, agent, true);
			Assert (status == History::Connected);
			XExecuteScriptCmds (cmdList, agent);
			// Store in the history only when script has a valid script id
			if (hdr.ScriptId () != gidInvalid)
				_history.XInsertExecutedMembershipScript (hdr, cmdList);
		}
	}
	agent.GetAckBox ().RememberAck (fullSynchSenderId, firstScriptId);
	dbg << "<-- Mailbox::XUnpackFullSynch" << std::endl;
}

// Returns true when all package scripts are successfully inserted into the history
bool Db::XUnpackPackage (ScriptList const & package, Mailbox::Agent & agent)
{
	bool scriptInserted = true;
	Assert (_projectDb.XGetMyId () != gidInvalid);
	for (ScriptList::Sequencer seq (package); !seq.AtEnd (); seq.Advance ())
	{
		ScriptHeader const & hdr = seq.GetHeader ();
		if (hdr.IsPackage ())
		{
			// REVISIT: add implementation
			// ScriptList const & package = seq.GetPackage ();
			// XUnpackPackage (package);
		}
		else
		{
			Assert (hdr.ScriptId () != gidInvalid);
			CommandList const & cmdList = seq.GetCmdList ();
			if (!_history.XInsertIncomingScript (hdr, cmdList, agent.GetAckBox ()))
				scriptInserted = false;
			if (hdr.GetUnitType () == Unit::Set)
				_projectDb.XUpdateSender (hdr.ScriptId ());
			agent.SetScriptUnpacked (true);
		}
	}
	return scriptInserted;
}

void Db::RetrieveScript (std::string const & path,
						 std::unique_ptr<ScriptHeader> & hdr,
						 std::unique_ptr<CommandList> & cmdList,
						 TransportHeader * txHdr,
						 DispatcherScript * dispatcher) const
{
	if (txHdr != 0 || dispatcher != 0)
	{
		FileDeserializer in (path);
		if (txHdr != 0)
		{
			// Read transport header
			txHdr->Read (in);
			if (!txHdr->IsValid ())
			{
				// Transport header not present -- rewind deserializer
				in.Rewind ();
			}
		}
		if (dispatcher != 0)
		{
			// Read Dispatcher script
			dispatcher->Read (in);
			// Dispatcher script is always added after the synch script -- rewind deserializer
			in.Rewind ();
		}
	}

	ScriptReader reader (path);
	hdr = reader.RetrieveScriptHeader ();
	cmdList = reader.RetrieveCommandList ();
}

bool Db::RetrieveVersionInfo (GlobalId scriptId, VersionInfo & info) const
{
	ScriptReader reader (GetScriptPath (scriptId));
	std::unique_ptr<ScriptHeader> hdr = reader.RetrieveScriptHeader ();
	info.SetComment (hdr->GetComment ());
	info.SetTimeStamp (hdr->GetTimeStamp ());
	info.SetVersionId (scriptId);
	return true;
}

std::string const & Db::GetScriptPath (GlobalId scriptId) const
{
	Assert (IsScriptFilePresent (scriptId));
	std::vector<ScriptInfo>::const_iterator iter =
		std::find_if (_inboxScripts.begin (), _inboxScripts.end (), IsEqualScriptId (scriptId));
	Assert (iter != _inboxScripts.end ());
	ScriptInfo const & info = *iter;
	return info.GetScriptPath ();
}

void Db::RetrieveJoinRequest (std::unique_ptr<ScriptHeader> & hdr,
							  std::unique_ptr<CommandList> & cmdList) const
{
	Assert (HasExecutableJoinRequest ());
	ScriptInfo const * info = GetJoinRequest ();
	Assert (info != 0);
	ScriptReader reader (info->GetScriptPath ().c_str ());
	hdr = reader.RetrieveScriptHeader ();
	cmdList = reader.RetrieveCommandList ();
}

void Db::XDeleteScript (GlobalId scriptId)
{
	std::vector<ScriptInfo>::iterator iter =
		std::find_if (_inboxScripts.begin (), _inboxScripts.end (), IsEqualScriptId (scriptId));
	if (iter != _inboxScripts.end ())
	{
		ScriptInfo const & info = *iter;
		File::DeleteNoEx (info.GetScriptPath ().c_str ());
		_inboxScripts.erase (iter);
	}
	else if (_sidetrack.IsMissing (scriptId))
	{
		_sidetrack.XRemoveMissingScript (scriptId);
		_history.XDeleteScript (scriptId);
	}
	else
	{
		// Delete already unpacked script
		_history.XDeleteScript (scriptId);
	}
	Notify (changeRemove, scriptId);
}

bool Db::FolderChange (FilePath const & folder) const
{
	return folder.IsEqualDir (_inboxPath);
}

void Db::BeginTransaction () 
{
	_cache.Invalidate ();
}

void Db::Clear () throw ()
{
	_inboxScripts.clear ();
	_inboxPath.Clear ();
}

class IsEqualPath : public std::unary_function<ScriptInfo const &, bool>
{
public:
	IsEqualPath (std::string const & path)
		: _path (path)
	{}
	bool operator () (ScriptInfo const & info) const
	{
		return info.GetScriptPath () == _path;
	}
private:
	std::string const &	_path;
};

// Returns true when there are no incoming script in the inbox
bool Db::Verify () const
{
	// List files present in the project inbox folder
	for (FileMultiSeq diskIter (_inboxPath, _scriptPatterns); !diskIter.AtEnd (); diskIter.Advance ())
	{
		std::string inboxPath (_inboxPath.GetFilePath (diskIter.GetName ()));
		std::vector<ScriptInfo>::const_iterator iter =
			std::find_if (_inboxScripts.begin (), _inboxScripts.end (), IsEqualPath (inboxPath));
		if (iter == _inboxScripts.end ())
		{
			// File present on disk is not recorded as script from the future or
			// wrongly addressed.
			return false;
		}
	}
	return true;
}
