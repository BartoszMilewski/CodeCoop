#if !defined (MAILREADER_H)
#define MAILREADER_H
//-----------------------------------------
// (c) Reliable Software 1998 -- 2004
//-----------------------------------------

#include "SimpleMapi.h"
#include "SimpleSession.h"
#include "EmailTransport.h"

class FilePath;
class SafePaths;

namespace SimpleMapi
{
	class MailboxIterator : public InboxIteratorInterface
	{
	public:
		MailboxIterator (Session & session, bool unreadOnly = true);
		void Advance ();
		bool AtEnd () const { return _done; }

		void RetrieveAttachements (SafePaths & attPaths);
		void GetSender (std::string & name, std::string & email) const;
		std::string const & GetSubject () const { return _msgSubject; }
		std::string GetMsgId () const { return _messageID; }
		void Seek (std::string const & msgId) throw ();
		bool DeleteMessage () throw ();
		void CleanupMessage () throw () { DeleteMessage (); }

	private:
		void RetrieveMessage ();

		Session &		_session;
		FLAGS			_findFlags;
		mutable char	_messageID [514];
		std::string		_msgSubject;
		bool			_done;

		MapiFindNext	_findNext;
		MapiReadMail	_read;
		MapiDeleteMail	_delete;
		MapiFree		_free;
	};
}

#endif
