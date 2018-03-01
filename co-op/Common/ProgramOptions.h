#if !defined (PROGRAMOPTIONS_H)
#define PROGRAMOPTIONS_H
//------------------------------------
//  (c) Reliable Software, 2003 - 2008
//------------------------------------

class Catalog;
// Forward declarations needed to establish these classes as belonging to
// the global scope (not ProgramOptions scope).
class ResendPageHndlr;
class UpdatePageHndlr;
class ChunkSizePageHndlr;
class ScriptConflictPageHndlr;
class InvitationsPageHndlr;

namespace Win
{
	class CritSection;
}

namespace ProgramOptions
{
	class Resend
	{
	public:
		Resend ();

		unsigned GetDelay () const { return _delay; }
		unsigned GetRepeatInterval () const { return _repeat; }
		bool ChangesDetected () const { return _changesDetected; }
		void ClearChanges () { _changesDetected = false; }

		void SetDelay (unsigned delay)
		{
			if (_delay != delay)
			{
				_delay = delay;
				_changesDetected = true;
			}
		}
		void SetRepeat (unsigned repeat)
		{
			if (_repeat != repeat)
			{
				_repeat = repeat;
				_changesDetected = true;
			}
		}

	private:
		unsigned long	_delay;
		unsigned long	_repeat;
		bool			_changesDetected;
	};

	class Update
	{
	public:
		Update ();

		bool IsAutoUpdate () const { return _isOn; }
		bool IsBackgroundDownload () const { return _isBackgroundDownload; }
		unsigned GetUpdateCheckPeriod () const { return _period; }

		bool ChangesDetected () const { return _changesDetected; }
		void ClearChanges () { _changesDetected = false; }

		void SetAutoUpdate (bool flag)
		{
			if (_isOn != flag)
			{
				_isOn = flag;
				_changesDetected = true;
			}
		}
		void SetBackgroundDownload (bool flag)
		{
			if (_isBackgroundDownload != flag)
			{
				_isBackgroundDownload = flag;
				_changesDetected = true;
			}
		}
		void SetUpdateCheckPeriod (unsigned period)
		{
			if (_period != period)
			{
				_period = period;
				_changesDetected = true;
			}
		}
	private:
		unsigned	_period;
		bool		_isOn;
		bool		_isBackgroundDownload;
		bool		_changesDetected;
	};

	class ChunkSize
	{
	public:
		ChunkSize (Catalog & catalog);

		unsigned long GetChunkSize () const { return _chunkSize; }
		bool ChangesDetected () const { return _changesDetected; }
		bool CanChange () const { return _canChange; }

		void ClearChanges () { _changesDetected = false; }
		void SetChunkSize (unsigned chunkSize)
		{
			if (_chunkSize != chunkSize)
			{
				_chunkSize = chunkSize;
				_changesDetected = true;
			}
		}

	private:
		unsigned long	_chunkSize;
		bool			_canChange;
		bool			_changesDetected;
	};

	class Invitations
	{
	public:
		Invitations (Catalog & catalog);

		Catalog & GetCatalog () { return _catalog; }
		bool ChangesDetected () const { return _changesDetected; }
		bool IsAutoInvitation () const { return _isAutoInvitation; }
		std::string const & GetProjectFolder () const { return _projectPath; }

		void ClearChanges () { _changesDetected = false; }
		void SetAutoInvite (bool flag);
		void SetAutoInvitePath (std::string const & path);

	private:
		bool		_changesDetected;
		bool		_isAutoInvitation;
		std::string	_projectPath;
		Catalog &	_catalog;
	};

	class ScriptConflict
	{
	public:
		ScriptConflict ();

		bool ChangesDetected () const { return _changesDetected; }
		bool IsResolveQuietly () const { return _isResolveQuietly; }

		void ClearChanges () { _changesDetected = false; }
		void SetResolveQuietly (bool flag);

	private:
		bool	_changesDetected;
		bool	_isResolveQuietly;
	};

	class Data
	{
	public:
		friend class ResendPageHndlr;
		friend class UpdatePageHndlr;
		friend class ChunkSizePageHndlr;
		friend class ScriptConflictPageHndlr;
		friend class InvitationsPageHndlr;

		Data (Catalog & catalog)
			: _chunkSize (catalog),
			  _invitations (catalog)
		{}

		bool ChangesDetected () const { return _resend.ChangesDetected () ||
											   _update.ChangesDetected () ||
											   _chunkSize.ChangesDetected () ||
											   _scriptConflict.ChangesDetected () ||
											   _invitations.ChangesDetected (); }
		void ClearChanges ()
		{
			_resend.ClearChanges ();
			_update.ClearChanges ();
			_chunkSize.ClearChanges ();
			_scriptConflict.ClearChanges ();
			_invitations.ClearChanges ();
		}

		Resend const & GetResendOptions () const { return _resend; }
		Update const & GetUpdateOptions () const { return _update; }
		ChunkSize const & GetChunkSizeOptions () const { return _chunkSize; }
		ScriptConflict const & GetScriptConflictOptions () const { return _scriptConflict; }
		Invitations const & GetInitationOptions () const { return _invitations; }

	private:
		Resend			_resend;
		Update			_update;
		ChunkSize		_chunkSize;
		ScriptConflict	_scriptConflict;
		Invitations		_invitations;
	};
}

std::ostream & operator<<(std::ostream & os, ProgramOptions::Data const & options);

#endif
