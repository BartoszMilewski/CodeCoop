#if !defined (PROJECTSTATE_H)
#define PROJECTSTATE_H
//------------------------------------
//  (c) Reliable Software, 1999 - 2007
//------------------------------------

namespace Project
{
	class State
	{
	public:
		State () {}
		State (unsigned long long value)
			: _bits (value)
		{}

		void SetCurrent (bool bit)			{ _bits.set (Current, bit); }
		void SetMail (bool bit)				{ _bits.set (IncomingScripts, bit); }
		void SetFullSync (bool bit)			{ _bits.set (AwaitingFullSync, bit); }
		void SetCheckedoutFiles (bool bit)	{ _bits.set (CheckedoutFiles, bit); }
		void SetUnavailable (bool bit)		{ _bits.set (Unavailable, bit);		}
		void SetUnderRecovery (bool bit)	{ _bits.set (UnderRecovery, bit); }

		unsigned long GetValue () const { return _bits.to_ulong (); }
	 
		bool IsCurrent () const				{ return _bits.test (Current); }
		bool HasMail () const				{ return _bits.test (IncomingScripts); }
		bool IsAwatingFullsync () const		{ return _bits.test (AwaitingFullSync); }
		bool HasCheckedoutFiles () const	{ return _bits.test (CheckedoutFiles); }
		bool IsUnavailable () const			{ return _bits.test (Unavailable);  }
		bool IsUnderRecovery () const		{ return _bits.test (UnderRecovery); }

	private:
		enum
		{
			Current,			// Currently visited project
			IncomingScripts,	// Incoming mail present
			AwaitingFullSync,	// Awaiting the full sync script
			CheckedoutFiles,	// Checked out files present
			Unavailable,		// Project unavailable
			UnderRecovery		// Project awaits verification package
		};

	private:
		std::bitset<std::numeric_limits<unsigned long>::digits>	_bits;
	};
}

#endif
