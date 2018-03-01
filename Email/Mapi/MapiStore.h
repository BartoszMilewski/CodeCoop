#if !defined (MAPISTORE_H)
#define MAPISTORE_H
//
// (c) Reliable Software 1998 -- 2001
//

#include "MapiProperty.h"
#include "MapiIface.h"
#include "MapiHelpers.h"

#include <mapix.h>

class OutgoingMessage;

namespace Mapi
{
	class Session;
	class RecipientList;
	class ReceiveFolderId;
	class FolderEntryId;

	class Folder
	{
	public:
		Folder () {}

		operator Com::UnknownPtr & () { return _folder; }
		operator void * () { return _folder; }

		void GetContentsTable (Interface<IMAPITable> & table);
		void CreateMessage (Interface<IMessage> & msg);
		void OpenMessage (std::vector<unsigned char> const & id, Interface<IMessage> & msg);

	protected:
		Interface<IMAPIFolder>	_folder;
	};

	class MsgStore
	{
	public:
		void OpenFolder (ReceiveFolderId & id, Folder & folder)
		{
			OpenFolder (id.GetSize (), id.Get (), folder);
		}
		void OpenFolder (FolderEntryId  & id, Folder & folder)
		{
			OpenFolder (id.GetSize (), id.Get (), folder);
		}
		void GetReceiveFolderId (EntryId & id);
		void GetFolderId (Buffer<SPropValue> & buf, unsigned long folderIdTag);
		bool IsOutboxFolderValid () const
		{
			return _folderMask.IsOutboxFolderValid ();
		}

		bool IsWasteBasketFolderValid () const
		{
			return _folderMask.IsWasteBasketFolderValid ();
		}

		bool IsSentItemsFolderValid () const
		{
			return _folderMask.IsSentItemsFolderValid ();
		}

	protected:
		MsgStore () {}

		void OpenFolder (unsigned long idSize, ENTRYID * id, Com::UnknownPtr & folder);

	protected:
		FolderMask				_folderMask;
		Interface<IMsgStore>	_store;
	};

	class MsgStoresTable
	{
	public:
		MsgStoresTable (Session & session);

		void GetDefaultStoreId (std::vector<unsigned char> & id);

	private:
		Interface<IMAPITable>	_table;
	};

	class DefaultMsgStore : public MsgStore
	{
	public:
		DefaultMsgStore (Session & session, MsgStoresTable & storesTable);
	};

	class Inbox : public Folder
	{
	public:
		Inbox (MsgStore & msgStore);
		void MoveMessage (std::vector<unsigned char> const & msgId, Folder & toFolder);
	};

	class Outbox : public Folder
	{
	public:
		Outbox (MsgStore & msgStore);

		void Submit (OutgoingMessage const & msg, RecipientList const & recipients, bool verbose);
		void Save (OutgoingMessage const & msg, RecipientList const & recipients);

	private:
		void BuildMessage (Message & msg, OutgoingMessage const & blueprint, RecipientList const & recipients);
	private:
		FolderEntryId	_sentItemsId;
	};

	class WasteBasket : public Folder
	{
	public:
		WasteBasket (MsgStore & msgStore);

		bool IsValid () const { return !_folder.IsNull (); }
	};

	class SentItems : public Folder
	{
	public:
		SentItems (MsgStore & msgStore);

		bool IsValid () const { return !_folder.IsNull (); }

		void GetSender (std::string & name, std::string & emailAddr);
	};
}

#endif
