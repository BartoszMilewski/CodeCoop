#if !defined (MAILBOXSCRIPTSTATE_H)
#define MAILBOXSCRIPTSTATE_H
//------------------------------------
//  (c) Reliable Software, 2003 - 2006
//------------------------------------

namespace Mailbox
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

		void SetForThisProject (bool bit) { _bits.set (ForThisProject, bit); }
		void SetFromFuture (bool bit) { _bits.set (FromFuture, bit); }
		void SetNext (bool bit) { _bits.set (Next, bit); }
		void SetJoinRequest (bool bit) { _bits.set (JoinRequest, bit); }
		void SetCorrupted (bool bit) { _bits.set (Corrupted, bit); }
		void SetIllegalDuplicate (bool bit) { _bits.set (IllegalDuplicate, bit); }
		void SetCannotForward (bool bit) { _bits.set (CannotForward, bit); }
		void SetMissing (bool bit) { _bits.set (Missing, bit); }

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

		bool IsForThisProject () const { return _bits.test (ForThisProject); }
		bool IsFromFuture () const { return _bits.test (FromFuture); }
		bool IsNext () const { return _bits.test (Next); }
		bool IsJoinRequest () const { return _bits.test (JoinRequest); }
		bool IsCorrupted () const { return _bits.test (Corrupted); }
		bool IsIllegalDuplicate () const { return _bits.test (IllegalDuplicate); }
		bool IsCannotForward () const { return _bits.test (CannotForward); }

	private:
		enum
		{
			Archive,			// Script placed in the archive
			Label,				// Label script -- project milestone marker
			Rejected,			// Rejected script
			Missing,			// Placeholder for missing script
			Executed,			// Executed script
			AutoExec,			// Automatic script execution
			BranchPoint,		// Branch point in the history
			Inventory,			// Complete set of project files

			// Don't change the order of the above enumeration items.
			// They match History::Note::State bits, so we can initialize
			// Mailbox::ScriptState with History::Note::State.

			// Specific to the mailbox view display
			ForThisProject,		// For this project -- global project names match
			FromFuture,			// Script from the future -- script lineage not connected
								// to the local lineage or script sender not known yet
			Next,				// Next script to be unpacked
			JoinRequest,		// Join request control script
			Corrupted,			// Corrupted script -- cannot be processed
			IllegalDuplicate,	// Illegal script duplicate -- same project, id already in history but command list different
			CannotForward		// Cannot forward script
		};

	private:
		std::bitset<std::numeric_limits<unsigned long>::digits>	_bits;
	};
}

#endif
