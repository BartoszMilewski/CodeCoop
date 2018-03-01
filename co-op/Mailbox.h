#if !defined MAILBOX_H
#define MAILBOX_H
//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "MailboxHelper.h"
#include "Transactable.h"
#include "Table.h"
#include "Global.h"
#include "MemberInfo.h"
#include "Lineage.h"
#include "Sidetrack.h"

#include <File/Path.h>
#include <File/Dir.h>

class PathFinder;
namespace History
{
	class Db;
}
namespace Project
{
	class Db;
}
class TransportHeader;
class DispatcherScript;
class TransactionFileList;
class ScriptMailer;
namespace Progress
{
	class Meter;
}
class ScriptHeader;
class CommandList;
class ScriptList;
class ScriptTrailer;
class Catalog;
class VersionInfo;

typedef std::map<std::string, std::string> CorruptedScriptMap;

//
// Mailbox
//

namespace Mailbox
{
	class ScriptFile;

	class Db : public SoftTransactable, public Table
	{
	public:
		Db (Project::Db & projectDb, History::Db &	history, Sidetrack & sidetrack);

		void InitPaths (PathFinder & pathFinder);

		bool ScriptFilesPresent () const;
		bool IsScriptFilePresent (GlobalId scriptId) const;
		bool IsJoinRequestScript (GlobalId scriptId) const;
		void RememberScript (std::string const & fileName, 
							Mailbox::ScriptState state, 
							std::string const & errorMsg);
		bool XUnpackScripts (Mailbox::Agent & agent, CorruptedScriptMap & corruptedScripts, Progress::Meter * meter);
		void XRememberCorruptedScript (ScriptInfo const & scriptInfo)
		{
			_inboxScripts.push_back (scriptInfo);
		}
		void RetrieveScript (std::string const & path,
							 std::unique_ptr<ScriptHeader> & hdr,
							 std::unique_ptr<CommandList> & cmdList,
							 TransportHeader * txHdr = 0,
							 DispatcherScript * dispatcher = 0) const;
		bool RetrieveVersionInfo (GlobalId scriptId, VersionInfo & info) const;
		std::string const & GetScriptPath (GlobalId scriptId) const;
		void RetrieveJoinRequest (std::unique_ptr<ScriptHeader> & hdr,
								  std::unique_ptr<CommandList> & cmdList) const;
		void XDeleteScript (GlobalId gid);
		bool FolderChange (FilePath const & folder) const;
		void Dump (std::ostream  & out) const;
		bool HasScripts () const { return !_inboxScripts.empty (); }
		bool HasScriptsFromFuture () const;
		bool HasExecutableJoinRequest () const;
		//
		// SoftTransactable interface
		//
		void BeginTransaction ();
		void CommitTransaction ()  throw () {};
		void AbortTransaction () {};
		void Clear () throw ();
		//
		// Table interface
		//
		void QueryUniqueIds (Restriction const & restrict, GidList & ids) const;
		Table::Id GetId () const { return Table::mailboxTableId; }
		bool IsValid () const { return true; }
		std::string GetStringField (Column col, GlobalId gid) const;
		std::string GetStringField (Column col, UniqueName const & uname) const;
		GlobalId GetIdField (Column col, UniqueName const & uname) const;
		GlobalId GetIdField (Column col, GlobalId gid) const;
		unsigned long GetNumericField (Column col, GlobalId gid) const;
		std::string GetCaption (Restriction const & restrict) const;
		// Returns true when there are no incoming script in the inbox
		bool Verify () const;

	public:
		class Sequencer
		{
		public:
			Sequencer (Mailbox::Db const & mailbox)
				: _cur (mailbox._inboxScripts.begin ()),
				  _end (mailbox._inboxScripts.end ())
			{}

			bool AtEnd () const { return _cur == _end; }
			void Advance () { ++_cur; }

			ScriptInfo const & GetScriptInfo () const { return *_cur; }

		private:
			std::vector<ScriptInfo>::const_iterator	_cur;
			std::vector<ScriptInfo>::const_iterator	_end;
		};

		friend class Sequencer;

	private:
		enum InsertDisposition
		{
			Insert,
			ExecuteOnly,
			Ignore,
			DontKnow
		};

	private:
		bool XCanUnpackScript (ScriptFile & script, Agent & agent);
		bool XUnpackScript (ScriptFile & script, Agent & agent); 
		void XUnpackChunk (ScriptFile & script, Agent & agent); 
		bool XInsertScript (ScriptFile & script, Agent & agent);
		Db::InsertDisposition XGetInsertDisposition (ScriptFile const & script) const;
		ScriptInfo const * GetJoinRequest () const;
		void XExecuteScriptTrailer (ScriptTrailer const & trailer, Mailbox::Agent & agent);
		void XExecuteScriptCmds (CommandList const & cmds, Mailbox::Agent & agent);
		void XUnpackFullSynch (ScriptList const & package, GlobalId fullSynchSenderId, Mailbox::Agent & agent);
		bool XUnpackPackage (ScriptList const & package, Mailbox::Agent & agent);

	private:
		MemberCache mutable		_cache;
		Project::Db &			_projectDb;
		History::Db &			_history;
		Sidetrack &				_sidetrack;
		FilePath				_inboxPath;
		std::vector<ScriptInfo>	_inboxScripts;		// Scripts from future or wrongly addressed
		FileMultiSeq::Patterns	_scriptPatterns;
	};
}

#endif
