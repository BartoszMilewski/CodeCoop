#if !defined (MAPIMAILREADER_H)
#define MAPIMAILREADER_H
//-----------------------------------------
// (c) Reliable Software 1998, 99, 2000, 01
//-----------------------------------------

#include "MapiDefDir.h"
#include "MapiStore.h"
#include "MapiContentsTable.h"
#include "MapiMessage.h"
#include "EmailTransport.h"

#include <Dbg/Assert.h>
#include <StringOp.h>

class FilePath;
class SafePaths;

namespace Mapi
{
	class MailboxIterator : public InboxIteratorInterface
	{
	public:
		MailboxIterator (Session & session, bool unreadOnly = true);
		void Advance ();
		bool AtEnd () const { return _cur >= _contentsSize; }

		void RetrieveAttachements (SafePaths & attPaths);
		void GetSender (std::string & name, std::string & email) const;
		std::string const & GetSubject () const
		{
			Assert (_curEnvelope.get () != 0);
			return _curEnvelope->GetSubject ();
		}
		std::string GetMsgId () const { return ToString (_cur); }
		void Seek (std::string const & msgId) throw ();
		bool DeleteMessage () throw ();
		void CleanupMessage () throw () { DeleteMessage (); }

	private:
		Mapi::Session &					_session;
		Mapi::DefaultDirectory			_defaultDir;
		Mapi::Inbox						_inbox;
		Mapi::ContentsTable				_contents;
		unsigned						_contentsSize;
		Mapi::WasteBasket				_deletedItems;
		unsigned int					_cur;
		std::unique_ptr<Mapi::Message>	_curMsg;
		std::unique_ptr<Mapi::Envelope>	_curEnvelope;
	};

};

#endif
