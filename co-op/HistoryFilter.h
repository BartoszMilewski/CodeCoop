#if !defined (HISTORYFILTER_H)
#define HISTORYFILTER_H
//------------------------------------
//  (c) Reliable Software, 2002 - 2008
//------------------------------------

#include "GlobalId.h"
#include "VerificationReport.h"

namespace Workspace
{
	class HistorySelection;
}
class FileCmd;

// History filtering predicate interface.

namespace History
{
	class Filter
	{
	public:
		Filter (Workspace::HistorySelection & selection, GlobalId startId)
			: _selection (selection),
			  _idVersionStart (startId)
		{
			Assert (_idVersionStart != gidInvalid);
		}
		virtual ~Filter () {}

		GlobalId GetStartId () const { return _idVersionStart; }
		// Returns true if a given script is relevant when scanning history
		virtual bool IsScriptRelevant (GlobalId scriptId) = 0;
		// Returns true if a given script command is relevant when scanning script
		virtual bool IsCommandRelevant (GlobalId fileGid) = 0;
		// Returns true if history scanning has been completed
		virtual bool AtEnd () const = 0;
		virtual void AddCommand (std::unique_ptr<FileCmd> cmd, GlobalId scriptId);

	protected:
		Workspace::HistorySelection &	_selection;
		GlobalId						_idVersionStart;
	};

	// Includes all file commands in the script range [idVersionStart, idVersionStop)
	class RangeAllFiles : public Filter
	{
	public:
		RangeAllFiles (Workspace::HistorySelection & selection,
					   GlobalId idVersionStart,
					   GlobalId idVersionStop)
			: Filter (selection, idVersionStart),
			  _idVersionStop (idVersionStop),
			  _startVersionSeen (idVersionStart == gidInvalid),
			  _atEnd (false)
		{}

		bool IsScriptRelevant (GlobalId scriptId);
		bool IsCommandRelevant (GlobalId fileGid) { return true; }
		bool AtEnd () const { return _atEnd; }

	private:
		GlobalId	_idVersionStop;
		bool		_startVersionSeen;
		bool		_atEnd;
	};

	// Includes some file commands in the script range [idVersionStart, idVersionStop)
	class RangeSomeFiles : public RangeAllFiles
	{
	public:
		RangeSomeFiles (Workspace::HistorySelection & selection,
						GlobalId idVersionStart,
						GlobalId idVersionStop,
						GidSet const & preSelectedGids)
			: RangeAllFiles (selection, idVersionStart, idVersionStop),
			  _preSelectedGids (preSelectedGids)
		{}

		bool IsCommandRelevant (GlobalId fileGid)
		{
			return _preSelectedGids.find (fileGid) != _preSelectedGids.end ();
		}

	private:
		GidSet const &	_preSelectedGids;
	};

	// Includes some file commands in a script until a new file command found
	class RepairFilter : public Filter
	{
	public:
		RepairFilter (Workspace::HistorySelection & selection,
					  VerificationReport::Sequencer corruptedFiles,
					  GlobalId startScriptId)
			: Filter (selection, startScriptId)
		{
			Assert (!corruptedFiles.AtEnd ());
			do
			{
				_preSelectedGids.insert (corruptedFiles.Get ());
				corruptedFiles.Advance ();
			} while (!corruptedFiles.AtEnd ());
		}

		bool IsScriptRelevant (GlobalId scriptId)
		{
			return true;
		}
		bool IsCommandRelevant (GlobalId fileGid)
		{
			return _preSelectedGids.find (fileGid) != _preSelectedGids.end ();
		}
		void AddCommand (std::unique_ptr<FileCmd> cmd, GlobalId scriptId);
		bool AtEnd () const { return _preSelectedGids.empty (); }

		void MoveUnrecoverableFiles (GidSet & unrecoverable);
	private:
		GidSet	_preSelectedGids;
	};

	// Includes one file commands and script ids until a new file command found
	class BlameFilter : public Filter
	{
	public:
		BlameFilter (Workspace::HistorySelection & selection,
					 GlobalId fileGid,
					 GidList & scriptIds,
					 GlobalId startScriptId)
			: Filter (selection, startScriptId),
			  _fileGid (fileGid),
			  _scriptIds (scriptIds),
			  _atEnd (false)
		{}

		bool IsScriptRelevant (GlobalId scriptId)
		{
			return true;
		}
		bool IsCommandRelevant (GlobalId fileGid)
		{
			return _fileGid == fileGid;
		}
		void AddCommand (std::unique_ptr<FileCmd> cmd, GlobalId scriptId);
		bool AtEnd () const { return _atEnd; }

	private:
		GlobalId	_fileGid;
		GidList &	_scriptIds;
		bool		_atEnd;
	};


	class FilterScanner
	{
	public:
		enum Token
		{
			tokenName,
			tokenFileId,
			tokenScriptId,
			tokenStarDotStar,
			tokenEnd
		};

	public:
		FilterScanner (std::string const & input)
			: _input (input),
			  _curPosition (0)
		{
			Accept ();
		}

		Token Look () const { return _curToken; }
		std::string GetTokenString ();
		void Accept ();

	private:
		static char const _whiteChars [];
		static char const _stopChars [];

		bool EatWhite ();
		void AcceptTokenString ();

	private:
		std::string const & _input;
		unsigned int		_curPosition;
		unsigned int		_curTokenStringStart;
		unsigned int		_curTokenStringEnd;
		Token				_curToken;
	};
}

#endif
