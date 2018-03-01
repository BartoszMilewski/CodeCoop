#if !defined (PROGRAMOPTIONSDLG_H)
#define PROGRAMOPTIONSDLG_H
//------------------------------------
//  (c) Reliable Software, 2003 - 2008
//------------------------------------

#include "ProgramOptions.h"
#include "resource.h"

#include <Ctrl/PropertySheet.h>
#include <Ctrl/Button.h>
#include <Ctrl/ComboBox.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Spin.h>
#include <Ctrl/Static.h>

class Catalog;
namespace Win
{
	class CritSection;
}

class ResendPageHndlr : public PropPage::Handler
{
public:
	ResendPageHndlr (ProgramOptions::Data & options)
		: PropPage::Handler (IDD_RESEND_OPTIONS),
		  _options (options)
	{}

	void OnCancel (long & result) throw (Win::Exception);
	void OnApply (long & result) throw (Win::Exception);

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned id, unsigned notifyCode) throw ();

private:
	Win::ComboBox			_delaySelector;
	Win::ComboBox			_repeatSelector;
	ProgramOptions::Data &	_options;
};

class UpdatePageHndlr : public PropPage::Handler
{
public:
	UpdatePageHndlr (ProgramOptions::Data & options)
		: PropPage::Handler (IDD_COOP_UPDATE_OPTIONS),
		  _options (options)
	{}

	void OnCancel (long & result) throw (Win::Exception);
	void OnApply (long & result) throw (Win::Exception);

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned id, unsigned notifyCode) throw ();

private:
	Win::CheckBox			_autoUpdate;
	Win::CheckBox			_inBackground;
	Win::Edit				_period;
	Win::Spin				_periodSpin;
	ProgramOptions::Data &	_options;
};

class ChunkSizePageHndlr : public PropPage::Handler
{
public:
	ChunkSizePageHndlr (ProgramOptions::Data & options)
		: PropPage::Handler (IDD_CHUNKING_OPTIONS),
		  _options (options)
	{}

	void OnCancel (long & result) throw (Win::Exception);
	void OnApply (long & result) throw (Win::Exception);

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned id, unsigned notifyCode) throw ();

private:
	Win::Edit				_size;
	Win::StaticText			_info;
	ProgramOptions::Data &	_options;
};

class InvitationsPageHndlr : public PropPage::Handler
{
public:
	InvitationsPageHndlr (ProgramOptions::Data & options)
		: PropPage::Handler (IDD_INVITATION_OPTIONS),
		  _options (options)
	{}

	void OnCancel (long & result) throw (Win::Exception);
	void OnApply (long & result) throw (Win::Exception);

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned id, unsigned notifyCode) throw ();

private:
	Win::CheckBox			_autoInvite;
	Win::Edit				_projectPath;
	Win::Button				_pathBrowse;
	ProgramOptions::Data &	_options;
};

class ScriptConflictPageHndlr : public PropPage::Handler
{
public:
	ScriptConflictPageHndlr (ProgramOptions::Data & options)
		: PropPage::Handler (IDD_SCRIPT_CONFLICT_OPTIONS),
		  _options (options)
	{}

	void OnCancel (long & result) throw (Win::Exception);
	void OnApply (long & result) throw (Win::Exception);

	bool OnInitDialog () throw (Win::Exception);

private:
	Win::CheckBox			_resolveQuietly;
	ProgramOptions::Data &	_options;
};

namespace ProgramOptions
{
	class HandlerSet : public PropPage::HandlerSet
	{
	public:
		HandlerSet (ProgramOptions::Data & programOptions);

		bool IsValidData () const { return _programOptions.ChangesDetected (); }

	private:
		ProgramOptions::Data &	_programOptions;
		ChunkSizePageHndlr		_chunkSizePageHndlr;
		ResendPageHndlr			_resendPageHndlr;
		ScriptConflictPageHndlr	_scriptConflictPageHndlr;
		UpdatePageHndlr			_updatePageHndlr;
		InvitationsPageHndlr	_invitationsPageHndlr;
	};
}
#endif
