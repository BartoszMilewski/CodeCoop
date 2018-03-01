//
// (c) Reliable Software 1998
//

#include "MapiStore.h"
#include "Message.h"
#include "MapiRecipientList.h"
#include "MapiSession.h"
#include "MapiEx.h"

#include <Sys\WinString.h>
#include <mapiutil.h>

//
// MAPI Message Store
//

MsgStoresTable::MsgStoresTable (Session & session)
{
	HRESULT hRes = session->GetMsgStoresTable (0, &_p);
	if (FAILED (hRes))
		throw MapiException ("Cannot get message stores table", hRes);
	MapiRows rows;
	hRes = ::HrQueryAllRows (_p,	// [in] Pointer to the MAPI table from which rows are retrieved. 
									 0,
													// [in] Pointer to an SPropTagArray structure containing an array
													// of property tags indicating table columns.
													// These tags are used to select the specific columns to be retrieved.
													// If the ptaga parameter is NULL, HrQueryAllRows retrieves the entire
													// column set of the current table view passed in the ptable parameter. 
									 0,		// [in] Pointer to an SRestriction structure containing retrieval restrictions.
													// If the pres parameter is NULL, HrQueryAllRows makes no restrictions
									 0,				// [in] Pointer to an SSortOrderSet structure identifying the sort order
													// of the columns to be retrieved. If the parameter is NULL,
													// the default sort order for the table is used. 
									 0,				// [in] Maximum number of rows to be retrieved.
													// If the value of the crowsMax parameter is zero,
													// no limit on the number of rows retrieved is set. 
									 rows.FillIn ());	// [out] Pointer to a pointer to the returned SRowSet structure containing
													// an array of pointers to the retrieved table rows. 
	if (FAILED (hRes))
		throw MapiException ("Cannot retrieve message stores table rows", hRes);
	rows.Dump ("Message Stores Table Rows");
}

DefaultMsgStoreID::DefaultMsgStoreID (MsgStoresTable & storesTable)
	: _resultRows (0)
{
	DefaultStoreFilter	filter;
	// Revisit: remove usage of this ugly macro below; we need variable size property tag array
	static SizedSPropTagArray(2, colDef) = { 2, PR_ENTRYID, PR_DEFAULT_STORE };
	HRESULT hRes = ::HrQueryAllRows (storesTable,	// [in] Pointer to the MAPI table from which rows are retrieved. 
									 reinterpret_cast<LPSPropTagArray>(&colDef),
													// [in] Pointer to an SPropTagArray structure containing an array
													// of property tags indicating table columns.
													// These tags are used to select the specific columns to be retrieved.
													// If the ptaga parameter is NULL, HrQueryAllRows retrieves the entire
													// column set of the current table view passed in the ptable parameter. 
									 &filter,		// [in] Pointer to an SRestriction structure containing retrieval restrictions.
													// If the pres parameter is NULL, HrQueryAllRows makes no restrictions
									 0,				// [in] Pointer to an SSortOrderSet structure identifying the sort order
													// of the columns to be retrieved. If the parameter is NULL,
													// the default sort order for the table is used. 
									 0,				// [in] Maximum number of rows to be retrieved.
													// If the value of the crowsMax parameter is zero,
													// no limit on the number of rows retrieved is set. 
									 &_resultRows);	// [out] Pointer to a pointer to the returned SRowSet structure containing
													// an array of pointers to the retrieved table rows. 
	if (FAILED (hRes))
		throw MapiException ("Cannot retrieve default message store id", hRes);
}

DefaultMsgStoreID::~DefaultMsgStoreID ()
{
	if (_resultRows != 0)
		::FreeProws (_resultRows);
}

bool DefaultMsgStoreID::Found () const
{ 
	return _resultRows != 0 &&
	       _resultRows->cRows != 0 &&
		   _resultRows->aRow [0].cValues != 0 &&
		   _resultRows->aRow [0].lpProps [0].ulPropTag == PR_ENTRYID;
}


MsgStore::MsgStore (Session & session, SBinary const & storeID)
{
	HRESULT hRes = session->OpenMsgStore (0,			// [in] Handle of the parent window for the common address dialog box and other related displays
										  storeID.cb,	// [in] Count of bytes in the entry identifier pointed to by the lpEntryID parameter
										  reinterpret_cast<ENTRYID *>(storeID.lpb),
														// [in] Pointer to the entry identifier of the message store to be opened.
														// This parameter must not be NULL.
										  0,			// [in] Pointer to the interface identifier (IID) representing the interface
														// to be used to access the message store. Passing NULL
														// results in a pointer to its standard interface being
														// returned in the out parameter. The standard interface for a message store is IMsgStore. 
										  MDB_NO_DIALOG | MDB_WRITE,
														// [in] Bitmask of flags that controls how the object is opened.
										  &_p);			// [out] Pointer to a pointer to the message store.
	if (FAILED (hRes))
		throw MapiException ("Cannot open default message store", hRes);
	//MapiFolder root (*this);
	//HierarchyTable hTable (root);
	//hTable.Dump (*this, "Message Store Root Hierarchy Table");
	//ContentsTable contents (root);
	//contents.Dump ("Message Store Root Contents Table");
}

bool MsgStore::IsOutboxFolderValid ()
{
	MapiBuffer<SPropValue> prop;
	GetProperty (prop, PR_VALID_FOLDER_MASK);
	bool outboxValid = (prop->Value.ul & FOLDER_IPM_OUTBOX_VALID != 0);
	return outboxValid;
}

void MsgStore::GetProperty (MapiBuffer<SPropValue> & prop, ULONG propTag)
{
	HRESULT hRes = HrGetOneProp (_p,					// [in] Pointer to the IMAPIProp interface from which the
														// property value is to be retrieved.
								 propTag,				// [in] Property tag of the property to be retrieved.
								 prop.GetFillBuf ());	// [out] Pointer to a pointer to the returned SPropValue structure
														// defining the retrieved property value.
	if (FAILED (hRes))
		throw MapiException ("Cannot access message store properties", hRes);
}

//
// MAPI Message Folders
//

Outbox::Outbox (Session & session)
{
	// Open the default message store
	MsgStoresTable storesTable (session);
	DefaultMsgStoreID defaultStoreID (storesTable);
	if (defaultStoreID.Found ())
	{
		MsgStore defaultMsgStore (session, defaultStoreID.GetEntryID ());
		if (defaultMsgStore.IsOutboxFolderValid ())
		{
			// Open the Outbox folder
			MapiBuffer<SPropValue> prop;
			defaultMsgStore.GetProperty (prop, PR_IPM_OUTBOX_ENTRYID);
			ULONG objectType;
			HRESULT hRes = defaultMsgStore->OpenEntry (prop->Value.bin.cb,// [in] Count of bytes in the entry identifier pointed to by the next parameter.
										reinterpret_cast<ENTRYID *>(prop->Value.bin.lpb),
																// [in] Pointer to the entry identifier of the object to open, or NULL.
																// If this parameter is set to NULL, OpenEntry opens the root folder for the message store.
										0,						// [in] Pointer to the interface identifier (IID) representing the interface
																// to be used to access the opened object. Passing NULL results
																// in the object's standard interface being returned which is
																// IMAPIFolder for folders and IMessage for messages.
										MAPI_MODIFY,			// Request read/write access to the folder
										&objectType,			// [out] Pointer to the type of the opened object.
										reinterpret_cast<struct IUnknown **>(&_p));
																// [out] Pointer to a pointer to the opened object.
			if (FAILED (hRes))
				throw MapiException ("Cannot open Outbox folder", hRes);
		}
		else
		{
			throw WinException ("The default message store doesn't contain valid Outbox folder");
		}
	}
	else
	{
		throw WinException ("Cannot find the default message store id");
	}
}

//
// MAPI message -- revisit more to a separate header file
//

class MapiMessage : public SIfacePtr<IMessage>
{
public:
	MapiMessage (Outbox & outbox);

	void AddRecipients (MapiAddrList const & addrList);
	void SetSubject (char const * subject);
	void SetClass (char const * str);
	void Submit ();

private:
	void SetProperty (SPropValue const * prop);
};

MapiMessage::MapiMessage (Outbox & outbox)
{
	HRESULT hRes = outbox->CreateMessage (0,	// [in] Pointer to the interface identifier (IID) representing
												// the interface to be used to access the New message.
												// Valid interface identifiers include IID_IUnknown,
												// IID_IMAPIProp, IID_IMAPIContainer, and IID_IMAPIFolder.
												// Passing NULL results in the message store provider returning
												// the standard message interface, IMessage.
										  0,	// [in] Bitmask of flags that controls how the message is created.
										  &_p);	// [out] Pointer to a pointer to the newly created message.
	if (FAILED (hRes))
		throw MapiException ("Cannot create outgoing message", hRes);
}

void MapiMessage::AddRecipients (MapiAddrList const & addrList)
{
	HRESULT hRes = _p->ModifyRecipients (MODRECIP_ADD,
										 addrList.GetBuf ());
	if (FAILED (hRes))
	{
		if (hRes == MAPI_E_EXTENDED_ERROR)
		{
			char const * extError = MapiExtendedError (hRes, _p);
			throw MapiException ("Cannot set message recipients", hRes, extError);
		}
		else
		{
			throw MapiException ("Cannot set message recipients", hRes);
		}
	}
}

void MapiMessage::SetSubject (char const * subject)
{
	SPropValue prop;
	memset (&prop, 0, sizeof (SPropValue));
	prop.ulPropTag = PR_SUBJECT;
	prop.Value.lpszA = const_cast<char *>(subject);
	SetProperty (&prop);
}

void MapiMessage::SetClass (char const * str)
{
	SPropValue prop;
	memset (&prop, 0, sizeof (SPropValue));
	prop.ulPropTag = PR_MESSAGE_CLASS;
	prop.Value.lpszA = const_cast<char *>(str);
	SetProperty (&prop);
}

void MapiMessage::Submit ()
{
	HRESULT hRes = _p->SubmitMessage (0);
	if (FAILED (hRes))
	{
		if (hRes == MAPI_E_EXTENDED_ERROR)
		{
			char const * extError = MapiExtendedError (hRes, _p);
			throw MapiException ("Cannot submit message", hRes, extError);
		}
		else
		{
			throw MapiException ("Cannot submit message", hRes);
		}
	}
}

void MapiMessage::SetProperty (SPropValue const * prop)
{
	HRESULT hRes = _p->SetProps (1,		// [in] Count of property values pointed to by the next parameter.
								 const_cast<SPropValue *>(prop),
										// [in] Pointer to an array of SPropValue structures holding
										// property values to be updated.
								 0);	// [in, out] On input, can be NULL, indicating no need for error
										// information, or a pointer to a pointer to an SPropProblemArray
										// structure. If this is a valid pointer on input, SetProps returns
										// detailed information about errors in updating one or more properties.
	if (FAILED (hRes))
	{
		if (hRes == MAPI_E_EXTENDED_ERROR)
		{
			char const * extError = MapiExtendedError (hRes, _p);
			throw MapiException ("Cannot set message property", hRes, extError);
		}
		else
		{
			throw MapiException ("Cannot set message property", hRes);
		}
	}
}

void Outbox::Submit (OutgoingMessage const & msg, RecipientList const & recipients, bool verbose)
{
	MapiMessage newMsg (*this);
	newMsg.SetSubject (msg.GetSubject ());
	newMsg.AddRecipients (recipients.GetMapiAddrList ());
	newMsg.SetClass ("IPM.Note");
	newMsg.Submit ();
}

HierarchyTable::HierarchyTable (MapiFolder & folder)
{
	HRESULT hRes = folder->GetHierarchyTable (0, &_p);
	if (FAILED (hRes))
		throw MapiException ("Cannot get folder hierarchy table", hRes);
}

void HierarchyTable::Dump (MsgStore & store, char const * title)
{
	static SizedSPropTagArray(4, colDef) = { 4, PR_DISPLAY_NAME, PR_DEPTH, PR_OBJECT_TYPE, PR_ENTRYID };
	MapiRows rows;
	HRESULT hRes = ::HrQueryAllRows (_p,	// [in] Pointer to the MAPI table from which rows are retrieved. 
							 reinterpret_cast<LPSPropTagArray>(&colDef),
									// [in] Pointer to an SPropTagArray structure containing an array
									// of property tags indicating table columns.
									// These tags are used to select the specific columns to be retrieved.
									// If the ptaga parameter is NULL, HrQueryAllRows retrieves the entire
									// column set of the current table view passed in the ptable parameter. 
							 0,		// [in] Pointer to an SRestriction structure containing retrieval restrictions.
									// If the pres parameter is NULL, HrQueryAllRows makes no restrictions
							 0,		// [in] Pointer to an SSortOrderSet structure identifying the sort order
									// of the columns to be retrieved. If the parameter is NULL,
									// the default sort order for the table is used. 
							 0,		// [in] Maximum number of rows to be retrieved.
									// If the value of the crowsMax parameter is zero,
									// no limit on the number of rows retrieved is set. 
							 rows.FillIn ());	// [out] Pointer to a pointer to the returned SRowSet structure containing
									// an array of pointers to the retrieved table rows. 
	if (FAILED (hRes))
		throw MapiException ("Cannot retrieve root folder hierarchy table rows", hRes);
	rows.Dump (title);
	for (int i = 0; i < rows.Count (); i++)
	{
		if (rows.IsFolder (i))
		{
			Msg info;
			info << "Folder '" << rows.GetDisplayName (i) << "' hierarchy table";
			MapiFolder folder (store, rows.GetEntryId (i));
			HierarchyTable hierarchy (folder);
			hierarchy.Dump (store, info.c_str ());
		}
	}
}

MapiFolder::MapiFolder (MsgStore & store)
{
	ULONG objectType;
	HRESULT hRes = store->OpenEntry (0,// [in] Count of bytes in the entry identifier
											// pointed to by the lpEntryID parameter. 
									0,		// [in] Pointer to the entry identifier of the object to open, or NULL.
											// If lpEntryID is set to NULL, OpenEntry opens the root folder for the
											// message store.
									 0,		// [in] Pointer to the interface identifier (IID) representing the interface
											// to be used to access the opened object. Passing NULL results in the
											// object's standard interface being returned which is IMAPIFolder for 
											// folders and IMessage for messages.
									 0,		// in] Bitmask of flags that controls how the object is opened.
									 &objectType,
											// [out] Pointer to the type of the opened object. 
									 reinterpret_cast<struct IUnknown **>(&_p));
											// [out] Pointer to a pointer to the opened object.
	if (FAILED (hRes))
		throw MapiException ("Cannot open root folder", hRes);
}

MapiFolder::MapiFolder (MsgStore & store, SBinary const * folderID)
{
	ULONG objectType;
	HRESULT hRes = store->OpenEntry (folderID->cb,// [in] Count of bytes in the entry identifier
											// pointed to by the lpEntryID parameter. 
									reinterpret_cast<ENTRYID *>(folderID->lpb),
											// [in] Pointer to the entry identifier of the object to open, or NULL.
											// If lpEntryID is set to NULL, OpenEntry opens the root folder for the
											// message store.
									 0,		// [in] Pointer to the interface identifier (IID) representing the interface
											// to be used to access the opened object. Passing NULL results in the
											// object's standard interface being returned which is IMAPIFolder for 
											// folders and IMessage for messages.
									 0,		// in] Bitmask of flags that controls how the object is opened.
									 &objectType,
											// [out] Pointer to the type of the opened object. 
									 reinterpret_cast<struct IUnknown **>(&_p));
											// [out] Pointer to a pointer to the opened object.
	if (FAILED (hRes))
		throw MapiException ("Cannot open folder", hRes);
}

ContentsTable::ContentsTable (MapiFolder & folder)
{
	HRESULT hRes = folder->GetContentsTable (0, &_p);
	if (FAILED (hRes))
		throw MapiException ("Cannot get folder contents table", hRes);
}

void ContentsTable::Dump (char const * title)
{
	static SizedSPropTagArray(1, colDef) = { 1, PR_DISPLAY_NAME};
	MapiRows rows;
	HRESULT hRes = ::HrQueryAllRows (_p,	// [in] Pointer to the MAPI table from which rows are retrieved. 
							 reinterpret_cast<LPSPropTagArray>(&colDef),
									// [in] Pointer to an SPropTagArray structure containing an array
									// of property tags indicating table columns.
									// These tags are used to select the specific columns to be retrieved.
									// If the ptaga parameter is NULL, HrQueryAllRows retrieves the entire
									// column set of the current table view passed in the ptable parameter. 
							 0,		// [in] Pointer to an SRestriction structure containing retrieval restrictions.
									// If the pres parameter is NULL, HrQueryAllRows makes no restrictions
							 0,		// [in] Pointer to an SSortOrderSet structure identifying the sort order
									// of the columns to be retrieved. If the parameter is NULL,
									// the default sort order for the table is used. 
							 0,		// [in] Maximum number of rows to be retrieved.
									// If the value of the crowsMax parameter is zero,
									// no limit on the number of rows retrieved is set. 
							 rows.FillIn ());	// [out] Pointer to a pointer to the returned SRowSet structure containing
									// an array of pointers to the retrieved table rows. 
	if (FAILED (hRes))
		throw MapiException ("Cannot retrieve root folder hierarchy table rows", hRes);
	rows.Dump (title);
}

