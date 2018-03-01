#if !defined (PROJECTOPTIONS_H)
#define PROJECTOPTIONS_H
//----------------------------------
// (c) Reliable Software 2005 - 2007
//----------------------------------

#include <Bit.h>

namespace Project
{
	class Options
	{
	public:
		bool IsWiki () const { return _bits.test (WikiProj); }
		bool IsAutoSynch () const { return _bits.test (AutoSynch); }
		bool IsAutoJoin () const { return _bits.test (AutoJoin); }
		bool IsKeepCheckedOut () const { return _bits.test (KeepCheckedOut); }
		bool IsDistribution () const { return _bits.test (Distribution); }
		bool IsNoBranching () const { return _bits.test (NoBranching); }
		bool IsAutoFullSynch () const { return _bits.test (AutoFullSynch); }
		bool UseBccRecipients () const { return _bits.test (BccScriptRecipients); }
		bool IsCheckoutNotification () const { return _bits.test (CheckoutNotification); }
		bool IsCheckProjectName () const { return _bits.test (CheckProjectName); }
		bool IsProjectAdmin () const { return _bits.test (CreatorIsAdmin); }
		bool IsReceiver () const { return _bits.test (CreatorIsReceiver); }
		bool IsNewFromHistory () const { return _bits.test (NewProjectFromHistory); }

		void SetWiki (bool bit) { _bits.set (WikiProj, bit); }
		void SetAutoSynch (bool bit) { _bits.set (AutoSynch, bit); }
		void SetAutoJoin (bool bit) { _bits.set (AutoJoin, bit); }
		void SetKeepCheckedOut (bool bit) { _bits.set (KeepCheckedOut, bit); }
		void SetDistribution (bool bit) { _bits.set (Distribution, bit); }
		void SetNoBranching (bool bit) { _bits.set (NoBranching, bit); }
		void SetAutoFullSynch (bool bit) { _bits.set (AutoFullSynch, bit); }
		void SetBccRecipients (bool bit) { _bits.set (BccScriptRecipients, bit); }
		void SetCheckoutNotification (bool bit) { _bits.set (CheckoutNotification, bit); }
		void SetCheckProjectName (bool bit) { _bits.set (CheckProjectName, bit); }
		void SetIsAdmin (bool bit) { _bits.set (CreatorIsAdmin, bit); }
		void SetIsReceiver (bool bit) { _bits.set (CreatorIsReceiver, bit); }
		void SetNewFromHistory (bool bit) { _bits.set (NewProjectFromHistory, bit); }

		void Clear () { _bits.init (0); }

	private:
		enum OptionBits
		{
			WikiProj,			// Has index.wiki in the root directory
			AutoSynch,			// Automatically execute all incoming set scripts (including full sync)
			AutoJoin,			// Automatically accept all incoming join requests
			KeepCheckedOut,		// After check-in keep files checked out
			Distribution,		// Distribution project
			NoBranching,		// Distribution receiver cannot create project branch
			AutoFullSynch,		// Automatically execute only the full sync script
			BccScriptRecipients,// Script recipients are placed on the one Bcc list, otherwise
								// scripts are separately addresses to single to recipient for
								// every project member. Valid only for distribution project.
			CheckoutNotification,// Notify other project members about checked out files/folders
			CheckProjectName,	// Check if project names is already used
			CreatorIsAdmin,		// Project creator becomes a new project administrator
			CreatorIsReceiver,	// Project receiver is creating the new project
			NewProjectFromHistory// Project is recreated from exported history
		};

	private:
		BitSet<OptionBits>	_bits;
	};

}

#endif
