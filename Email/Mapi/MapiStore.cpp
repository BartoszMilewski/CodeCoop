//
// (c) Reliable Software 1998 -- 2004
//

#include "precompiled.h"
#include "MapiStore.h"
#include "MapiBuffer.h"
#include "MapiRestriction.h"
#include "MapiRecipientList.h"
#include "MapiSession.h"
#include "MapiMessage.h"
#include "MapiProperty.h"
#include "MapiContentsTable.h"
#include "EmailMessage.h"
#include "MapiEx.h"
#include "MapiGPF.h"

#include <mapiutil.h>
#include <mapival.h>

namespace Mapi
{

//
// MAPI Folder
//

void Folder::GetContentsTable (Interface<IMAPITable> & table)
{
	Result result;
	try
	{
		result = _folder->GetContentsTable (0, &table);
	}
	catch (...)
	{
		Mapi::HandleGPF ("IMAPIFolder::GetContentsTable");
		throw;
	}
	_folder.ThrowIfError (result, "MAPI -- Cannot get folder contents table.");
}

void Folder::CreateMessage (Interface<IMessage>	& msg)
{
	Result result;
	try
	{
		result = _folder->CreateMessage (0,	// [in] Pointer to the interface identifier (IID) representing
										// the interface to be used to access the New message.
										// Valid interface identifiers include IID_IUnknown,
										// IID_IMAPIProp, IID_IMAPIContainer, and IID_IMAPIFolder.
										// Passing NULL results in the message store provider returning
										// the standard message interface, IMessage.
									0,	// [in] Bitmask of flags that controls how the message is created.
									&msg);
										// [out] Pointer to a pointer to the newly created message.
	}
	catch (...)
	{
		Mapi::HandleGPF ("IMAPIFolder::CreateMessage");
		throw;
	}
	_folder.ThrowIfError (result, "MAPI -- Cannot create message.");
}

void Folder::OpenMessage (std::vector<unsigned char> const & id, Interface<IMessage> & msg)
{
	ObjectType objectType;
	Com::UnknownPtr & unknown = msg;
	Result result;
	try
	{
		result = _folder->OpenEntry (id.size (),	// [in] Count of bytes in the entry identifier pointed to by the lpEntryID parameter.
									reinterpret_cast<ENTRYID *>(const_cast<unsigned char *>(&id [0])),
												// [in] Pointer to the entry identifier of the object to open, or NULL.
												// If lpEntryID is set to NULL, OpenEntry opens the root folder for the message store.
									0,			// [in] Pointer to the interface identifier (IID) representing the interface to be used to access the opened object.
												// Passing NULL results in the object's standard interface being returned which is
												// IMAPIFolder for folders and IMessage for messages.
									MAPI_BEST_ACCESS,
									&objectType,// [out] Pointer to the type of the opened object.
									&unknown);	// [out] Pointer to a pointer to the opened object.
	}
	catch (...)
	{
		Mapi::HandleGPF ("IMAPIFolder::OpenEntry");
		throw;
	}
	_folder.ThrowIfError (result, "MAPI -- Cannot open message.");
	Assert (objectType.IsMessage ());
}

//
// MAPI Message Store
//

void MsgStore::GetReceiveFolderId (EntryId & id)
{
	Result result;
	try
	{
		result = _store->GetReceiveFolder ("IPM.Note",// [in] Pointer to a message class that is associated with a receive folder.
													  // If the parameter is set to NULL or an empty string, GetReceiveFolder returns the default receive folder for the message store.
											0,			// [in] Bitmask of flags that controls the type of the passed-in and returned strings.
											&id._size,	// [out] Pointer to the count of bytes in the entry identifier pointed to by the next parameter.
											&id._id,	// [out] Pointer to a pointer to the entry identifier for the requested receive folder.
											0);		// [out] Pointer to a pointer to the message class that explicitly sets
													// as its receive folder the folder pointed to by lppEntryID. This message class should
													// either be the same as the class in the lpszMessageClass parameter or of a superclass of that class.
													// Passing NULL indicates that the folder pointed to by lppEntryID is the default receive
													// folder for the message store.
	}
	catch (...)
	{
		Mapi::HandleGPF ("IMsgStore::GetReceiveFolder");
		throw;
	}
	_store.ThrowIfError (result, "MAPI -- Cannot retrieve receive folder id.");
}

void MsgStore::GetFolderId (Buffer<SPropValue> & buf, unsigned long folderIdTag)
{
	Result result;
	try
	{
		result = ::HrGetOneProp (_store,		// [in] Pointer to the IMAPIProp interface from which the
												// property value is to be retrieved.
							 folderIdTag,		// [in] Property tag of the property to be retrieved.
							 &buf);				// [out] Pointer to a pointer to the returned SPropValue structure
												// defining the retrieved property value.
	}
	catch (...)
	{
		Mapi::HandleGPF ("::HrGetOneProp (IMsgStore)");
		throw;
	}
	_store.ThrowIfError (result, "MAPI -- Cannot access folder entry id.");
}

MsgStoresTable::MsgStoresTable (Session & session)
{
	session.GetMsgStoresTable (_table);
	dbg << "Open Message Stores Table. refcount = " << _table.GetRefCount () << std::endl;
	Assert (_table.GetRefCount () == 1);
}

void MsgStoresTable::GetDefaultStoreId (std::vector<unsigned char> & id)
{
	DefaultStoreFilter	filter;
	PropertyList columns;
	columns.Add (PR_ENTRYID);
	columns.Add (PR_DEFAULT_STORE);
	RowSet resultRows;
	Result result;
	try
	{
		result = ::HrQueryAllRows (_table,	// [in] Pointer to the MAPI table from which rows are retrieved. 
							   columns.Cnv2TagArray (),
										// [in] Pointer to an SPropTagArray structure containing an array
										// of property tags indicating table columns.
										// These tags are used to select the specific columns to be retrieved.
										// If the ptaga parameter is NULL, HrQueryAllRows retrieves the entire
										// column set of the current table view passed in the ptable parameter. 
							   &filter,	// [in] Pointer to an SRestriction structure containing retrieval restrictions.
										// If the pres parameter is NULL, HrQueryAllRows makes no restrictions
							   0,		// [in] Pointer to an SSortOrderSet structure identifying the sort order
										// of the columns to be retrieved. If the parameter is NULL,
										// the default sort order for the table is used. 
							   0,		// [in] Maximum number of rows to be retrieved.
										// If the value of the crowsMax parameter is zero,
										// no limit on the number of rows retrieved is set. 
							   &resultRows);
										// [out] Pointer to a pointer to the returned SRowSet structure containing
										// an array of pointers to the retrieved table rows. 
	}
	catch (...)
	{
		Mapi::HandleGPF ("::HrQueryAllRows (IMsgStore)");
		throw;
	}
	_table.ThrowIfError (result, "MAPI -- Cannot retrieve default message store id.");
	Assert (!::FBadRowSet (resultRows.Get ()));
	Assert (resultRows.Count () == 1);
	SBinary const * bin = resultRows.GetEntryId (0);
	id.resize (bin->cb);
	memcpy (&id [0], bin->lpb, bin->cb);
}

DefaultMsgStore::DefaultMsgStore (Session & session, MsgStoresTable & storesTable)
{
	// Query message stores table for the default message store id
	std::vector<unsigned char> id;
	storesTable.GetDefaultStoreId (id);
	// Open default message store
	session.OpenMsgStore (id, _store);

	// Retrieve folder mask
	RetrievedProperty values;
	PropertyList props;
	props.Add (PR_VALID_FOLDER_MASK);
	dbg << "Mapi Session: GetProps" << std::endl;
	Result result;
	try
	{
		result = _store->GetProps (props.Cnv2TagArray (),
												// [in] Pointer to an array of property tags
												// identifying the properties whose values are to be retrieved.
									  0,		// [in] Bitmask of flags that indicates the format for properties
												// that have the type PT_UNSPECIFIED.
									  values.GetCountBuf (),
												// [out] Pointer to a count of property values pointed
												// to by the lppPropArray parameter.
									  values.GetBuf ());
												// [out] Pointer to a pointer to the retrieved property values.
	}
	catch (...)
	{
		Mapi::HandleGPF ("IMsgStore::GetProps");
		throw;
	}
	dbg << "Mapi Session: GetProps done" << std::endl;
	if (result.IsOk ())
	{
		Assert (values.GetCount () == 1);
		if (values [0].Value.ul != MAPI_E_NOT_FOUND)
			_folderMask = values [0].Value.ul;
	}
	Assert (_store.GetRefCount () == 1);
}

void MsgStore::OpenFolder (unsigned long idSize, ENTRYID * id, Com::UnknownPtr & folder)
{
	dbg << "Mapi MsgStore: OpenFolder" << std::endl;
	ObjectType objectType;
	Result result;
	try
	{
		result = _store->OpenEntry (idSize,	// [in] Count of bytes in the entry identifier pointed to by the lpEntryID parameter.
								   id,		// [in] Pointer to the entry identifier of the object to open, or NULL.
											// If lpEntryID is set to NULL, OpenEntry opens the root folder for the message store.
								   0,		// [in] Pointer to the interface identifier (IID) representing the interface to be used to access the opened object.
											// Passing NULL results in the object's standard interface being returned which is
											// IMAPIFolder for folders and IMessage for messages.
								   MAPI_BEST_ACCESS | MAPI_MODIFY,
											// Request read/write access to the folder
								   &objectType,
											// [out] Pointer to the type of the opened object.
								   &folder);
											// [out] Pointer to a pointer to the opened object.
	}
	catch (...)
	{
		Mapi::HandleGPF ("IMsgStore::OpenEntry");
		throw;
	}
	_store.ThrowIfError (result, "MAPI -- Cannot open message store folder.");
	dbg << "Mapi MsgStore: OpenFolder done" << std::endl;
	Assert (objectType.IsFolder ());
}

//
// MAPI Default Message Store Folders
//

//
// Receive folder -- inbox
//

Inbox::Inbox (MsgStore & msgStore)
{
	// Open receive folder
	ReceiveFolderId receiveFolderId (msgStore);
	msgStore.OpenFolder (receiveFolderId, *this);
	Assert (_folder.GetRefCount () == 1);
}

void Inbox::MoveMessage (std::vector<unsigned char> const & msgId, Folder & toFolder)
{
	SBinary id;
	id.cb = msgId.size ();
	id.lpb = const_cast<unsigned char *>((&msgId [0]));
	ENTRYLIST movedMsg;
	movedMsg.cValues = 1;
	movedMsg.lpbin = &id;
	Result result;
	try
	{
		result = _folder->CopyMessages (&movedMsg,	// [in] Pointer to an array of ENTRYLIST structures that
											// identify the message or messages to copy or move.
							   0,			// [in] Pointer to the interface identifier (IID) representing the interface
											// to be used to access the destination folder pointed to by the lpDestFolder
											// parameter. Passing NULL results in the service provider returning the
											// standard folder interface, IMAPIFolder. Clients must pass NULL.
							   toFolder,	// [in] Pointer to the open folder to receive the copied or moved messages.
							   0,			// [in] Handle of the parent window for any dialog boxes or windows this method displays.
											// The ulUIParam parameter is ignored unless the client sets the MESSAGE_DIALOG
											// flag in the ulFlags parameter and passes NULL in the lpProgress parameter.
							   0,			// [in] Pointer to a progress object for displaying a progress indicator.
											// If NULL is passed in lpProgress, the message store provider displays a progress
											// indicator using the MAPI progress object implementation. The lpProgress parameter is ignored
											// unless the MESSAGE_DIALOG flag is set in ulFlags.
							   MESSAGE_MOVE);
											// The message or messages are to be moved rather than copied.
											// If MESSAGE_MOVE is not set, the messages are copied.
	}
	catch (...)
	{
		Mapi::HandleGPF ("IMAPIFolder::CopyMessages");
		throw;
	}
	_folder.ThrowIfError (result, "MAPI -- Cannot move message.");
}

//
// Outgoing messages folder -- outbox
//

Outbox::Outbox (MsgStore & msgStore)
{
	if (msgStore.IsOutboxFolderValid ())
	{
		// Open the Outbox folder
		FolderEntryId outboxId (msgStore, PR_IPM_OUTBOX_ENTRYID);
		msgStore.OpenFolder (outboxId, *this);
		Assert (_folder.GetRefCount () == 1);
		if (msgStore.IsSentItemsFolderValid ())
		{
			_sentItemsId.Init (msgStore, PR_IPM_SENTMAIL_ENTRYID);
		}
	}
	else
	{
		throw Win::Exception ("The default message store doesn't contain valid Outbox folder.");
	}
}

void Outbox::Submit (OutgoingMessage const & msg, RecipientList const & recipients, bool verbose)
{
	Message newMsg (*this);
	BuildMessage (newMsg, msg, recipients);
	newMsg.Submit ();
}

void Outbox::Save (OutgoingMessage const & msg, RecipientList const & recipients)
{
	Message newMsg (*this);
	BuildMessage (newMsg, msg, recipients);
	newMsg.SaveChanges ();
}

void Outbox::BuildMessage (Message & msg, OutgoingMessage const & blueprint, RecipientList const & recipients)
{
	msg.SetSubject (blueprint.GetSubject ());
	msg.SetNoteText (blueprint.GetText ());
	msg.AddRecipients (recipients.GetAddrList ());
	if (blueprint.GetAttachmentCount () > 0)
	{
		for (OutgoingMessage::AttPathIter iter = blueprint.AttPathBegin (); iter != blueprint.AttPathEnd (); ++iter)
		{
			msg.AddAttachment (*iter);
		}
		msg.MarkHasAttachment ();
	}
	// After sending message copy it to the Sent Items folder
	msg.SetSentOptions (_sentItemsId);
}
//
// Deleted messages folder -- deleted items
//

WasteBasket::WasteBasket (MsgStore & msgStore)
{
	if (msgStore.IsWasteBasketFolderValid ())
	{
		// Open the Deleted Items folder
		FolderEntryId wasteBasketId (msgStore, PR_IPM_WASTEBASKET_ENTRYID);
		msgStore.OpenFolder (wasteBasketId, *this);
		Assert (_folder.GetRefCount () == 1);
	}
}

//
// Sent messages folder -- sent items
//

SentItems::SentItems (MsgStore & msgStore)
{
	if (msgStore.IsSentItemsFolderValid ())
	{
		// Open the Deleted Items folder
		FolderEntryId sentItemsId (msgStore, PR_IPM_SENTMAIL_ENTRYID);
		msgStore.OpenFolder (sentItemsId, *this);
		Assert (_folder.GetRefCount () == 1);
	}
}

void SentItems::GetSender (std::string & name, std::string & emailAddr)
{
	Assert (SentItems::IsValid ());
	ContentsTable contents (*this, false);	// All messages
	if (contents.size () != 0)
	{
		// Thera are sent messages -- check who send them
		Message lastSentMsg (*this, contents [contents.size () - 1]);
		Envelope lastEnvelope (lastSentMsg);
		name = lastEnvelope.GetSenderName ();
		emailAddr = lastEnvelope.GetSenderEmail ();
	}
}

}
