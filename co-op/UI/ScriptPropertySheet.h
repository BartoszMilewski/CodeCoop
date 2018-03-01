#if !defined (SCRIPTPROPERTYSHEET_H)
#define SCRIPTPROPERTYSHEET_H
//----------------------------------
// (c) Reliable Software 2002 - 2006
// ---------------------------------

#include "FileDiffDisplay.h"
#include "Resource.h"

#include <Ctrl/PropertySheet.h>
#include <Ctrl/Controls.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Static.h>
#include <Ctrl/ListView.h>
#include <Ctrl/Button.h>

class ScriptProps;

class MissingScriptPageHndlr : public PropPage::Handler
{
public:
	MissingScriptPageHndlr (ScriptProps const & props)
		: PropPage::Handler (IDD_MISSING_SCRIPT),
		  _props (props)
	{}

protected:
	bool OnInitDialog () throw (Win::Exception);

private:
	Win::EditReadOnly	_sender;
	Win::EditReadOnly	_senderId;
	Win::EditReadOnly	_senderHubId;
	Win::EditReadOnly	_scriptId;
	Win::EditReadOnly	_scriptType;
	Win::EditReadOnly	_chunkCount;
	Win::EditReadOnly	_maxChunkSize;
	Win::EditReadOnly	_receivedChunkCount;
	Win::ReportListing	_resendRecipients;
	Win::StaticText		_nextRecipientCaption;
	Win::ReportListing	_nextRecipient;
	ScriptProps const &	_props;
};

class ScriptGeneralPageHndlr : public PropPage::Handler
{
public:
	ScriptGeneralPageHndlr (int pageId, ScriptProps const & props)
		: PropPage::Handler (pageId),
		  _props (props)
	{}

protected:
	bool OnInitDialog () throw (Win::Exception);

protected:
	Win::EditReadOnly	_sender;
	Win::EditReadOnly	_senderId;
	Win::EditReadOnly	_senderHubId;
	Win::EditReadOnly	_scriptId;
	Win::EditReadOnly	_date;
	ScriptProps const &	_props;
};

class ChangeScriptHeaderPageHndlr : public ScriptGeneralPageHndlr
{
public:
	ChangeScriptHeaderPageHndlr (ScriptProps const & props)
		: ScriptGeneralPageHndlr (IDD_SCRIPT_HEADER, props)
	{}

	bool OnInitDialog () throw (Win::Exception);

private:
	Win::EditReadOnly	_comment;
	Win::ReportListing	_lineage;
	Win::StaticText		_memberFrame;
	Win::ReportListing	_members;
};

class FileDetailsPageHndlr : public PropPage::Handler
{
public:
	FileDetailsPageHndlr (FileDisplayTable const & fileTable)
		: PropPage::Handler (IDD_FILE_DETAILS),
		  _fileDisplay (IDC_FILE_DETAILS_LIST, const_cast<FileDisplayTable &>(fileTable))
	{}

	Notify::Handler * GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ();
	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned id, unsigned notifyCode) throw ();

private:
	FileDiffDisplay		_fileDisplay;
	Win::Button			_open;
	Win::Button			_openAll;
};

class MembershipUpdatePageHndlr : public ScriptGeneralPageHndlr
{
public:
	MembershipUpdatePageHndlr (ScriptProps const & props)
		: ScriptGeneralPageHndlr (IDD_INCOMING_MEMBERSHIP_UPDATE, props)
	{}

	bool OnInitDialog () throw (Win::Exception);

private:
	Win::EditReadOnly	_comment;
	Win::ReportListing	_lineage;
	Win::ReportListing	_changes;
};

class CtrlScriptPageHndlr : public ScriptGeneralPageHndlr
{
public:
	CtrlScriptPageHndlr (ScriptProps const & props)
		: ScriptGeneralPageHndlr (IDD_CTRL_SCRIPT, props)
	{}

	bool OnInitDialog () throw (Win::Exception);

private:
	void BuildCaption (std::string & caption) const;
	void FormatChunkInfo (std::string & caption) const;

private:
	Win::EditReadOnly	_comment;
	Win::StaticText		_caption;
};


class ScriptPropertyHndlrSet : public PropPage::HandlerSet
{
public:
	ScriptPropertyHndlrSet (ScriptProps const & props);

private:
	MissingScriptPageHndlr				_missingScriptPageHndlr;
	ChangeScriptHeaderPageHndlr			_scriptHdrPageHndlr;
	std::unique_ptr<FileDetailsPageHndlr>	_filePageHndlr;
	MembershipUpdatePageHndlr			_membershipUpdatePageHdnlr;
	CtrlScriptPageHndlr					_ctrlScriptPageHdnlr;
};

#endif
