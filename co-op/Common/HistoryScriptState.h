#if !defined (HISTORYSCRIPTSTATE_H)
#define HISTORYSCRIPTSTATE_H
//------------------------------------
//  (c) Reliable Software, 2003 - 2007
//------------------------------------

namespace History
{
	// Script state used by the UI
	class ScriptState
	{
	public:
		ScriptState ()
			: _bits (0)
		{}
		ScriptState (unsigned long long value)
			: _bits (value)
		{}

		void SetCurrent (bool bit) { _bits.set (Current, bit); }
		void SetProjectCreationMarker (bool bit) { _bits.set (ProjectCreationMarker, bit); }
		void SetInteresting (bool bit) { _bits.set (InterestingScript, bit); }
		void SetConfirmed (bool bit) { _bits.set (Confirmed, bit); }
		void SetMyInventory (bool bit) { _bits.set (MyInventory, bit); }

		unsigned long GetValue () const { return _bits.to_ulong (); }

		bool IsArchive () const	{ return _bits.test (Archive); }
		bool IsLabel () const	{ return _bits.test (Label); }
		bool IsRejected () const { return !_bits.test (Executed) && _bits.test (Rejected); }
		bool IsMissing () const { return _bits.test (Missing); }
		bool IsExecuted () const { return _bits.test (Executed) && !_bits.test (Rejected); }
		bool IsAutoExec () const { return _bits.test (AutoExec); }
		bool IsToBeRejected () const { return _bits.test (Executed) && _bits.test (Rejected); }
		bool IsIncoming () const { return !_bits.test (Executed) && !_bits.test (Rejected); }
		bool IsBranchPoint () const { return _bits.test (BranchPoint); }
		bool IsInventory () const { return _bits.test (Inventory); }

		bool IsCurrent () const	{ return _bits.test (Current); }
		bool IsProjectCreationMarker () const { return _bits.test (ProjectCreationMarker); }
		bool IsInteresting () const { return _bits.test (InterestingScript); }
		bool IsConfirmed () const { return _bits.test (Confirmed); }
		bool IsDeepFork () const { return _bits.test (DeepFork); }
		bool IsMyInventory () const { return _bits.test (MyInventory); }

	private:
		enum
		{
			Archive,			// Script placed in the archive
			Label,				// Label script -- project milestone marker
			Rejected,			// Rejected script
			Missing,			// Placeholder for missing script
			Executed,			// Script is ready for execution
			AutoExec,			// Automatic script execution
			BranchPoint,		// Branch point in the history
			Inventory,			// Complete set of project files

			// Don't change the order of the above enumeration items.
			// They match History::Note::State bits, so we can initialize
			// History::ScriptState with History::Note::State.

			Current,			// Script representing current project version
			ProjectCreationMarker,
			InterestingScript,	// Selectable script
			Confirmed,			// Confirmed script	
			DeepFork,			// Marks the original FIS in the branch project
			MyInventory			// Initial file inventory was create by me
		};

	private:
		std::bitset<std::numeric_limits<unsigned long>::digits>	_bits;
	};
}

#endif
