#if !defined (HISTORY_H)
#define HISTORY_H
//------------------------------------
//	(c) Reliable Software, 1997 - 2008
//------------------------------------

#include "ScriptKind.h"
#include "Params.h"
#include "Table.h"
#include "XArray.h"
#include "SerString.h"
#include "MultiLine.h"
#include "Global.h"
#include "Lineage.h"
#include "XLong.h"
#include "XFileOffset.h"
#include "MemberInfo.h"
#include "HistorySortedTree.h"
#include "ScriptList.h"

#include <TimeStamp.h>

#include <iosfwd>

class FileData;
class PathFinder;
class FilePath;
namespace Progress
{
	class Meter;
}
class ExportedHistoryTrailer;
class FileFilter;
class ProgressMeter;
class VersionInfo;
class ProgressMeter;
class ScriptProps;
class ScriptHeader;
class CommandList;
class ScriptCmd;
class FileCmd;
class MemoryLog;
class AckBox;
namespace Project
{
	class Db;
	class Options;
}
namespace Mailbox
{
	class Agent;
}

namespace History
{
	class Filter;
	class ScriptState;
	class Range;

	enum Status
	{
		Connected,		// All script lineages connect with history
		Disconnected,	// At least one script lineage is disconnected
		Prehistoric		// Main script lineage is prehistoric, side lineages can be prehistoric, connected or disconnected
	};

	enum Verification
	{
		Ok, Membership, Creation
	};

	class Db : public TransactableContainer, public Table
	{
		friend class Path;

		// Embedded classes

	private:
		class CmdLog : public Log<ScriptCmd>
		{
		public:
			std::unique_ptr<ScriptCmd> Retrieve (File::Offset logOffset, int version) const;
			std::unique_ptr<FileCmd> RetrieveFileCmd (File::Offset logOffset, int version) const;
		};

		class HdrNote;
		class XConversionSequencer;

		friend class XConversionSequencer;

		class Cache
		{
		public:
			Cache (Db const & history) 
				: _history (history),
				  _lastGid (gidInvalid),
				  _note (0),
				  _timeStamp (0)
			{}
			void Invalidate () { _lastGid = gidInvalid; }
			Node const * GetNode (GlobalId gid)
			{
				Assert (gid != gidInvalid);
				Refresh (gid);
				return _note;
			}
			std::string GetComment (GlobalId gid)
			{
				Refresh (gid);
				return _comment.GetFirstLine ();
			}
			std::string GetFullComment (GlobalId gid);
			std::string const & GetTimeStampStr (GlobalId gid)
			{
				Refresh (gid);
				Assert (!_note->IsMissing ());
				return _timeStamp.GetString ();
			}
			long GetTimeStamp (GlobalId gid)
			{
				Refresh (gid);
				Assert (!_note->IsMissing ());
				return _timeStamp.AsLong ();
			}
		private:
			void Refresh (GlobalId gid);

			Db const &			_history;
			GlobalId			_lastGid;
			Node const *		_note;
			std::unique_ptr<HdrNote> _hdr;
			MultiLineComment	_comment;
			StrTime 			_timeStamp;
		};

		friend Cache;
	public:
		// Must be public, because it's used in IsEqualMissingScriptId
		class MissingScriptId: public Serializable
		{
		public:
			MissingScriptId (GlobalId scriptId, Unit::Type unitType, GlobalId unitId)
				: _scriptId (scriptId),
				  _unitType (unitType),
				  _unitId (unitId)
			{}
			MissingScriptId (Deserializer& in, int version)
			{
				Deserialize (in, version);
			}

			GlobalId Gid () const { return _scriptId; }
			Unit::Type Type () const { return _unitType; }
			GlobalId UnitId () const { return _unitId; }

			// Serializable interface

			void Serialize (Serializer& out) const;
			void Deserialize (Deserializer& in, int version);

		private:
			GlobalId	_scriptId;	// it contains sender ID
			Unit::Type	_unitType;
			GlobalId	_unitId;	// what member is modified by this script
		};

	public:
		Db (Project::Db & dataBase);

		void InitPaths (SysPathFinder& pathFinder);
		bool Verify (Verification what) const;
		void XDoRepair (History::Verification what);
		bool VerifyMembership () const;
		void XDoRepairMembership ();
		void LeaveProject ();
		void XDefect ();
		void XCleanupMemberTree (UserId userId);
		bool IsFullSyncExecuted () const { return _initialFileInventoryIsExecuted; }
		bool XIsFullSyncUnpacked ();

		bool XIsRecorded (GlobalId scriptGid,
						  History::ScriptState & scriptState,
						  Unit::Type unitType = Unit::Set);

		bool CanDelete (GlobalId scriptId) const;
		bool IsNext (GlobalId scriptId) const;
		bool IsCandidateForExecution (GlobalId scriptId) const;
		bool IsExecuted (GlobalId scriptId) const;
		bool IsMissing (GlobalId scriptId) const;
		bool HasMissingPredecessors (GlobalId scriptId) const;
		bool IsRejected (GlobalId scriptId) const;
		bool IsBranchPoint (GlobalId scriptId) const;
		bool IsAtTreeTop (GlobalId scriptId) const;
		bool IsCurrentVersion (GlobalId scriptId) const;
		bool CanArchive (GlobalId scriptId, Unit::Type unitType = Unit::Set) const;
		bool CanDeleteMissing (GlobalId scriptId, std::string & whyCannotDelete) const;

		void XDeleteScript (GlobalId scriptId);
#if !defined (NDEBUG)
		void XMarkUndoneScript (GlobalId scriptId);
#endif
		void XMarkUndone (GlobalId scriptId);
		void XMarkMissing (GlobalId scriptId);
		void XMarkExecuted (ScriptHeader const & hdr);
		void XUpdateNextMarker ();

		static char const * GetCmdLogName () { return _cmdLogName; }
		GlobalId MostRecentScriptId () const;
		GlobalId XMostRecentScriptId ();
		GlobalId GetPredecessorId (GlobalId scriptId, Unit::Type unitType = Unit::Set) const;
		GlobalId GetFirstExecutedSuccessorId (GlobalId scriptId) const;
		GlobalId GetTrunkBranchPointId (GlobalId rejectedScriptId) const;
		void XGetLineages (ScriptHeader & hdr, UnitLineage::Type sideLineageType, bool includeReceivers = false);
		bool CheckCurrentLineage () const;
		void GetCurrentLineage (Lineage & lineage) const;
		void GetLineageStartingWith (Lineage & lineage, GlobalId refVersionId) const;
		void XGetLineageStartingWith (Lineage & lineage, GlobalId refVersionId);
		void GetUnitLineage (Unit::Type type, UserId id, Lineage & lineage) const;
		void XGetUnitLineage (Unit::Type type, UserId id, Lineage & lineage);
		void XGetMainSetLineage (Lineage & lineage);
		void GetTentativeScripts (ScriptList & fullSynch, Progress::Meter & meter) const;
		void GetMembershipScriptIds (GidList const & members, auto_vector<UnitLineage> & sideLineages);
		void GetMembershipScripts (ScriptList & fullSynch,
								   auto_vector<UnitLineage>::const_iterator sideLineagesSeq,
								   auto_vector<UnitLineage>::const_iterator sideLineagesEnd);
		void PatchMembershipScripts (GidList const & members,
									ScriptList::EditSequencer & scriptList, 
									GlobalId setReferenceId);
		void XRetrieveDefectedTrees (GidList const & knownDeadMembers, ScriptList & scriptList);
		void MapMostRecentScripts (std::map<UserId, GlobalId> & userToMostRecent);
		void GetToBeRejectedScripts (History::Range & range) const;
		void GetItemsYoungerThen (GlobalId scriptId, GidSet & youngerItems) const;

		// Retrieve history view version description (version, created by, date, script id, state, type)
		std::string RetrieveVersionDescription (GlobalId versionGid) const;
		std::string RetrieveLastRejectedScriptComment () const;

		bool Export (std::string const & filePath, Progress::Meter & progressMeter) const;
		bool VerifyImport (ExportedHistoryTrailer & trailer) const;
		void XImport (ExportedHistoryTrailer const & trailer,
					  std::string const & srcPath,
					  GidSet & changedFiles,
					  Progress::Meter & progressMeter,
					  bool resetFisMarker);

		void FindAllByComment (std::string const & keyword,
							   GidList & scripts,
							   GidSet & files) const;
		bool IsFileChangedByScript (GlobalId fileGid, GlobalId scriptGid) const;
		bool IsFileCreatedByScript (GlobalId fileGid, GlobalId scriptGid) const;
		void GetFilesChangedByScript (GlobalId scriptId, GidSet & files) const;
		bool HasIncomingOrMissingScripts () const;
		bool HasToBeRejectedScripts () const;
		bool HasNextScript () const;
		bool NextIsFullSynch () const;
		bool NextIsMilestone () const;
		unsigned int GetIncomingScriptCount () const;
		bool HasMissingScripts () const;
		bool HasMembershipUpdatesFromFuture () const;
		void GetMissingScripts (Unit::ScriptList & scriptIdList) const;
		void GetDisconnectedMissingSetScripts (GidList & scripts) const;

		Status XProcessLineages (ScriptHeader const & hdr, 
								 Mailbox::Agent & agent, 
								 bool processMainLineage);
		Status XProcessUnitLineage (UserId senderId, 
									UnitLineage const & lineage, 
									Mailbox::Agent & agent);
		bool XInsertIncomingScript (ScriptHeader const & hdr, CommandList const & cmdList, AckBox & ackBox);
		void XSubstituteScript (ScriptHeader const & hdr, CommandList const & cmdList, AckBox & ackBox);
		void XInsertExecutedMembershipScript (ScriptHeader const & hdr, CommandList const & cmdList);
		void XAddMissingInventoryMarker (GlobalId inventoryId);
		void XRemoveMissingInventoryMarker ();
		void XPushBackScript (ScriptHeader const & hdr, CommandList const & cmdList);
		void XAddCheckinScript (ScriptHeader const & hdr, CommandList const & cmdList, AckBox & ackBox, bool isInventory = false);
		void XAddProjectCreationMarker (ScriptHeader const & hdr, CommandList const & cmdList);
		void XFixProjectCreationMarker(std::string const & projectName, GlobalId scriptId);
		void XAcceptAck (GlobalId scriptId, UserId senderId, Unit::Type unitType, AckBox & ackBox, bool broadcastMakeRef = true);
		void XRemoveFromAckList (UserId userId, bool isDead, AckBox & ackBox);
		void XRemoveDisconnectedScript (GlobalId scriptId, Unit::Type unitType);
		bool IsArchive () const { return _isArchive; }
		void XArchive (GlobalId scriptId);
		void XUnArchive ();
		void XPrepareForBranch (MemberDescription const & branchCreator,
								std::string const & branchProjectName,
								GlobalId branchVersionId,
								Project::Options const & options);
		void XCleanupMembershipUpdates ();
		void XRetrieveFileData (GidSet & historicalFiles,
								std::vector<FileData> & fileData);
		void RetrieveNextVersionInfo (VersionInfo & info) const
		{
			RetrieveVersionInfo (_nextScriptId.Get (), info);
		}
		bool RetrieveVersionInfo (GlobalId scriptGid, VersionInfo & info) const;
		std::string RetrieveNextScriptCaption () const;
		bool RetrieveForcedScript (ScriptHeader & hdr, CommandList & cmdList) const;
		bool RetrieveNextScript (ScriptHeader & hdr, CommandList & cmdList) const;
		bool RetrieveScript (GlobalId scriptGid, ScriptHeader & hdr, CommandList & cmdList, Unit::Type unitType = Unit::Set) const;
		bool XRetrieveScript (GlobalId scriptGid, ScriptHeader & hdr, CommandList & cmdList, Unit::Type unitType = Unit::Set);
		bool XRetrieveCmdList (GlobalId scriptId, CommandList & cmdList, Unit::Type unitType = Unit::Set);
		void XRetrieveThisUserMembershipUpdateHistory (ScriptList & scriptList);
		bool RetrieveScript (GlobalId scriptGid, FileFilter const * filter, ScriptProps & props) const;
		void RetrieveSetCommands (Filter & filter, Progress::Meter & meter) const;
		std::string RetrieveComment (GlobalId scriptGid) const;
		std::string GetSenderName (GlobalId scriptGid) const;
		void CreateRangeFromCurrentVersion (GlobalId stopScriptId,
											GidSet const & fileFilter,
											Range & range) const;
		GlobalId CreateRangeToFirstExecuted (GlobalId startScriptId,
											 GidSet const & fileFilter,
											 Range & range) const;
		void CreateRangeFromTwoScripts (GlobalId firstScriptId,
										GlobalId secondScriptId,
										GidSet const & fileFilter,
										Range & range) const;
		void GetForkIds (GidList & forkIds, bool deepForks = false) const;
		GlobalId CheckForkIds (GidList const & otherProjectForkIds,
							   bool deepForks,
							   GidList & myYoungerForkIds) const;
		void PruneDeadMemberList(GidList & deadMemberList);
		void CopyLog (FilePath const & destPath) const;
		void CreateLogFiles ();
		void VerifyLogs () const;

		//	For repair and diagnosis of corrupt history
		void DumpFileChanges (std::ostream & out) const;
		void DumpMembershipChanges (std::ostream & out) const;
		void DumpDisconnectedScripts (std::ostream & out) const;
		void DumpCmdLog (std::ostream & out) const;
		void XCorrectItemUniqueName (GlobalId itemGid, UniqueName const & correctUname);


		// Version 4.2 to 4.5 conversions
		void XPreConversionDump (MemoryLog & log);
		void XPostConversionDump (MemoryLog & log);
		void XConvert (Progress::Meter * meter, MemoryLog & log);
		void XCollectScriptIds (std::map<UserId, std::pair<GlobalId, GlobalId> > & userScripts,
								Progress::Meter * meter,
								MemoryLog & log);
		
		// Version 4.5 to 4.6 conversion
		void XMembershipTreeCleanup ();

		// Transactable interface

		void BeginTransaction ();

		void Serialize (Serializer& out) const;
		void Deserialize (Deserializer& in, int version);

		bool IsSection () const { return true; }
		int  SectionId () const { return 'HIST'; }
		int  VersionNo () const { return modelVersion; }

		// Table interface

		void QueryUniqueIds (Restriction const & restrict, GidList & ids) const;
		Table::Id GetId () const { return Table::historyTableId; }
		bool IsValid () const;
		std::string GetStringField (Column col, GlobalId gid) const;
		std::string GetStringField (Column col, UniqueName const & uname) const;
		GlobalId GetIdField (Column col, UniqueName const & uname) const;
		GlobalId GetIdField (Column col, GlobalId gid) const;
		unsigned long GetNumericField (Column col, GlobalId gid) const;
		std::string GetCaption (Restriction const & restrict) const;

		void GetUnpackedScripts (GidList & ids) const;

	private:
		class ScriptRestriction
		{
		public:
			ScriptRestriction (FileFilter const * fileFilter, CmdLog const & cmdLog);
			ScriptRestriction (GidSet const & fileFilter, CmdLog const & cmdLog);

			bool IsScriptRelevant (Node const * node);

		private:
			GidSet			_currentFileFilter;
			bool			_noFilter;
			CmdLog const &	_cmdLog;
		};

	private:
		void XRemoveDuplicatesInDifferentTrees ();
		void CommnaLineSelection (GidList const & preSelectedIds, GidList & selectedIds) const;
		void FilteredQuery (Restriction const & restrict, GidList & selectedIds) const;
		std::unique_ptr<Node> XLogMyRemoval (ScriptHeader const & hdr, 
										   CommandList const & cmdList);
		std::unique_ptr<Node> XLogMemberRemoval (ScriptHeader const & hdr,
											   CommandList const & cmdList,
											   AckBox & ackBox);
		void XRemoveFromAckList (SortedTree & tree, UserId userId, bool isDead, AckBox & ackBox);
		void XSetPredecessorIds (MemoryLog * log = 0, Progress::Meter * meter = 0);
		void ScanTree (SortedTree const & tree, GlobalId startingId, std::map<UserId, GlobalId> & mostRecentScriptIds) const;
		SortedTree const & GetTree (Unit::Type unitType) const;
		SortedTree & XGetTree (Unit::Type unitType);
		void RetrieveHeader (Node const * node, ScriptHeader & hdr) const;
		void RetrieveCommandList (Node const * node, CommandList & cmdList) const;
		void XGetRecordedUnitIds (Unit::Type type, GidList & unitIds) const;
		void GetReverseLineage (GidList & reverseLineage, GlobalId refVersionId) const;
		void XGetReverseLineage (GidList & reverseLineage, GlobalId refVersionId);
		std::unique_ptr<Node> XLogCheckinScript (ScriptHeader const & hdr, CommandList const & cmdList);
		std::unique_ptr<Node> XLogIncomingScript (ScriptHeader const & hdr, 
												CommandList const & cmdList, 
												bool isConfirmed, 
												AckBox & ackBox);
		std::unique_ptr<Node> XLogScript (ScriptHeader const & hdr, CommandList const & cmdList, GidList const & ackList);
		void RetrieveScript (Node const * node, ScriptHeader & hdr, CommandList & cmdList) const;
		void InitScriptHeader (Node const * node, Unit::Type unitType, CommandList const & cmdList, ScriptHeader & hdr) const;
		bool XRetrieveCmdList (Node const * node, CommandList & cmdList) const;
		std::string GetLastResendRecipient (GlobalId scriptId) const;
		bool XIsDuplicateScript (Node const * node,
								 CommandList const & scriptCmdList) const;
		void XRememberDisconnectedScript (GlobalId scriptId, Unit::Type unitType, GlobalId unitId);
		void XInsertLabelScript (ScriptHeader const & hdr); // Will be used for inserting labels into history
		void XMarkDeepFork (GlobalId scriptId)
		{
			_setChanges.XMarkDeepFork (scriptId);
		}
		bool XPrecedes (GlobalId precederId, GlobalId followerId) const
		{
			return _setChanges.XPrecedes (precederId, followerId);
		}
		void XRelinkTop();
		void DumpNode (std::ostream & out,
					   Node const * node,
					   bool dumpComment = false,
					   bool dumpCmdDetails = false) const;

	private:
		static char						_cmdLogName [];
		static char						_hdrLogName [];

		Project::Db &					_projectDb;

		SortedTree						_setChanges;			// Project set change list
		XLongWithDefault<gidInvalid>	_nextScriptId;

		SortedTree						_membershipChanges;		// Project membership change list

		TransactableArray<MissingScriptId>	_disconnectedScripts;

		CmdLog							_cmdLog;
		Log<HdrNote>					_hdrLog;
		XFileOffset						_cmdValidEnd;
		XFileOffset						_hdrValidEnd;

		// Volatile
		Cache mutable					_cache;
		MemberCache mutable				_memberCache;
		bool							_isArchive;
		bool							_initialFileInventoryIsExecuted;
	};

	//--------------------------------------------
	// Definitions of classes embedded in History
	//--------------------------------------------

	class Db::HdrNote : public Serializable
	{
	public:
		HdrNote (ScriptHeader const & hdr);
		HdrNote (Deserializer& in, int version)
		{
			Deserialize (in, version);
		}

		void ResetLineage (Lineage const & lineage) { _lineage.ReInit (lineage); }
		long GetTimeStamp () const { return _timeStamp; }
		GlobalId GetReferenceId () const { return _lineage.GetReferenceId (); }
		Lineage const & GetLineage () const { return _lineage; }
		std::string const & GetComment () const { return _comment; }

		// Serializable interface

		void Serialize (Serializer& out) const;
		void Deserialize (Deserializer& in, int version);

	private:
		long			_timeStamp;
		Lineage 		_lineage;
		SerString		_comment;
	};
}

std::ostream & operator<<(std::ostream & os, History::Db::MissingScriptId id);

#endif
