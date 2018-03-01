//-----------------------------------
// (c) Reliable Software 1998 -- 2004
//-----------------------------------
#include "precompiled.h"
#include "MapiMailReader.h"

#include <File/SafePaths.h>
#include <Dbg/Assert.h>

using namespace Mapi;

MailboxIterator::MailboxIterator (Session & session, bool unreadOnly)
	: _session (session),
	  _defaultDir (session),
	  _inbox (_defaultDir.GetMsgStore ()),
	  _contents (_inbox, unreadOnly),
	  _contentsSize (0), // will be set in Advance
	  _deletedItems (_defaultDir.GetMsgStore ()),
	  _cur (-1)
{
	Advance ();
	dbg << "Mapi Mailbox Iterator initialized" << std::endl;
}

void MailboxIterator::Advance ()
{
	Assert (_cur == -1 || _cur < _contentsSize);
	++_cur;
	_contentsSize = _contents.size ();
	if (_cur < _contentsSize)
	{
		_curMsg.reset (new Message (_inbox, _contents [_cur]));
		_curEnvelope.reset (new Envelope (*_curMsg));
	}
}

void MailboxIterator::Seek (std::string const & msgId) throw ()
{
	Assert (!msgId.empty ());
	std::istringstream in (msgId);
	in >> _cur;
	Assert (_cur < _contentsSize);
	_curMsg.reset (new Message (_inbox, _contents [_cur]));
	_curEnvelope.reset (new Envelope (*_curMsg));
}

void MailboxIterator::RetrieveAttachements (SafePaths & attPaths)
{
	Assert (_curMsg.get () != 0);
	AttachmentTable attachTable (*_curMsg);
	attachTable.SaveAttachments (attPaths);
}

void MailboxIterator::GetSender (std::string & name, std::string & email) const
{
	Assert (_curEnvelope.get () != 0);
	name.assign (_curEnvelope->GetSenderName ());
	email.assign (_curEnvelope->GetSenderEmail ());
}

// return true if client can continue iteration
bool MailboxIterator::DeleteMessage () throw ()
{
	_curMsg->MarkAsRead ();
	if (_deletedItems.IsValid ())
		_inbox.MoveMessage (_contents [_cur], _deletedItems);
	_curMsg.reset (0);
	_curEnvelope.reset (0);
	return false;
}
