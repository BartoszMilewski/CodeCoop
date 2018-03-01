//
// (c) Reliable Software 1998 -- 2004
//

#include "precompiled.h"
#include "MapiSession.h"
#include "MapiAddrBook.h"
#include "MapiEx.h"
#include "MapiGPF.h"

#include <Dbg/Assert.h>

using namespace Mapi;

MAPIINIT_0 Use::_mapiInit = { 0, MAPI_MULTITHREAD_NOTIFICATIONS };

Use::Use (bool isMultithreaded)
{
	dbg << "MapiInitialize" << std::endl;
	HRESULT hRes;
	if (isMultithreaded)
		hRes = ::MAPIInitialize (&_mapiInit);
	else
		hRes = ::MAPIInitialize (0);

	if (FAILED (hRes))
		throw Win::InternalException ("Cannot initialize MAPI subsystem");
	dbg << "MapiInitialize done" << std::endl;
}

Use::~Use ()
{
	::MAPIUninitialize ();
	dbg << "MapiUninitialize" << std::endl;
}

Session::Session (bool browseOnly)
	:_usesMapi (true)
{
	FLAGS logonFlags = MAPI_USE_DEFAULT |	// The messaging subsystem should use the default profile
					   MAPI_LOGON_UI |		// A dialog box should be displayed to prompt the user for logon information if required
					   MAPI_NEW_SESSION |	// An attempt should be made to create a new MAPI session rather than acquire the shared session.
					   MAPI_EXTENDED;		// Log on with extended capabilities
	if (browseOnly)
		logonFlags |= MAPI_NO_MAIL;			// MAPI should not inform the MAPI spooler of the session's existence.
											// The result is that no messages can be sent or received within the session
											// except through a tightly coupled store and transport pair. A calling client
											// sets this flag if it is acting as an agent, if configuration work must be done,
											// or if the client is browsing the available message stores.

	dbg << "MapiLogon" << std::endl;
	HRESULT hRes = ::MAPILogonEx (0,// [in] Handle to the window to which the logon dialog box is modal.
									// If no dialog box is displayed during the call the parameter is ignored.
									// This parameter can be zero.
								  "",// [in] Pointer to a string containing the name of the profile to use when logging on.
									// This string is limited to 64 characters.
								  "",// [in] Pointer to a string containing the password of the profile.
									// The parameter can be NULL whether or not the lpszProfileName parameter is NULL.
									// This string is limited to 64 characters.
								  logonFlags,
								  &_session);
	if (FAILED (hRes))
		throw Mapi::Exception ("MAPI -- Cannot logon using default profile.", hRes);
	Assert (!_session.IsNull());
	dbg << "MapiLogon done: refcount = " << _session.GetRefCount () << std::endl;
}

Session::~Session ()
{
	if (!_session.IsNull ())
	{
		try
		{
			_session->Logoff (0, 0, 0);
		}
		catch (...)
		{
			Mapi::HandleGPF ("IMAPISession::Logoff");
		}
		dbg << "Log Off. refcount = " << _session.GetRefCount () << std::endl;
	}
}

void Session::OpenAddressBook (Interface<IAddrBook> & addrBook)
{
	Result result;
	try
	{
		_session->OpenAddressBook (0,	// [in] Handle of the parent window for the common address
										// dialog box and other related displays.
								   0,	// [in] Pointer to the interface identifier (IID) representing the interface to be
										// used to access the address book. Passing NULL results in a pointer
										// to the address book's standard interface, or IAddrBook, being returned.
								   AB_NO_DIALOG, // Suppresses the display of dialog boxes.
								   &addrBook);   // [out] Pointer to a pointer to the address book.
	}
	catch (...)
	{
		Mapi::HandleGPF ("IMAPISession::OpenAddressBook");
		throw;
	}
	_session.ThrowIfError (result, "MAPI -- Cannot open address book.");
}

void Session::GetMsgStoresTable (Interface<IMAPITable> & table)
{
	Result result;
	try
	{
		dbg << "Mapi Session: GetMsgStoresTable" << std::endl;
		Assert (!_session.IsNull());
		result = _session->GetMsgStoresTable (0, &table);
	}
	catch (...)
	{
		Mapi::HandleGPF ("IMAPISession::GetMsgStoresTable.");
		throw;
	}
	_session.ThrowIfError (result, "MAPI -- Cannot get message stores table.");
}

void Session::GetStatusTable (Interface<IMAPITable> & table)
{
	Result result;
	try
	{
		_session->GetStatusTable (0, &table);
	}
	catch (...)
	{
		Mapi::HandleGPF ("IMAPISession::GetStatusTable");
		throw;
	}
	_session.ThrowIfError (result, "MAPI -- Cannot get session status table.");
}

void Session::OpenMsgStore (std::vector<unsigned char> const & id, Interface<IMsgStore> & store)
{
	dbg << "Mapi Session: OpenMsgStore" << std::endl;
	Result result;
	try
	{
		result = _session->OpenMsgStore (0,	// [in] Handle of the parent window for the common address dialog box and other related displays
									id.size (),
											// [in] Count of bytes in the entry identifier pointed to by the lpEntryID parameter
									reinterpret_cast<ENTRYID *>(const_cast<unsigned char *>(&id [0])),
											// [in] Pointer to the entry identifier of the message store to be opened.
											// This parameter must not be NULL.
									0,	// [in] Pointer to the interface identifier (IID) representing the interface
											// to be used to access the message store. Passing NULL
											// results in a pointer to its standard interface being
											// returned in the out parameter. The standard interface for a message store is IMsgStore. 
									MDB_NO_DIALOG | MDB_WRITE,
											// [in] Bitmask of flags that controls how the object is opened.
									&store);
											// [out] Pointer to a pointer to the message store.
	}
	catch (...)
	{
		Mapi::HandleGPF ("IMAPISession::OpenMsgStore");
		throw;
	}

	if (result.OneProviderFailed ())
		throw Win::InternalException ("MAPI -- transient problem.");
	_session.ThrowIfError (result, "MAPI -- Cannot open default message store.");
	dbg << "Mapi Session: OpenMsgStore done" << std::endl;
}

Result Session::OpenEntry (EntryId const & id, Com::UnknownPtr & unknown, ObjectType & objType)
{
	try
	{
		return _session->OpenEntry (id.GetSize (),	// [in] Count of bytes in the entry identifier pointed to by the lpEntryID parameter.
							id.Get (),		// [in] Pointer to the entry identifier of the object to open.
							0,				// [in] Pointer to the interface identifier (IID) representing
												// the interface to be used to access the opened object.
												// Passing NULL results in the object's standard interface being returned.
							MAPI_BEST_ACCESS,
												// in] Bitmask of flags that controls how the object is opened.
							&objType,			// [out] Pointer to the type of the opened object.
							&unknown);		// [out] Pointer to a pointer to the opened object.
	}
	catch (...)
	{
		Mapi::HandleGPF ("IMAPISession::OpenEntry");
		throw;
	}
}

Result Session::QueryIdentity (EntryId & id)
{
	try
	{
		return _session->QueryIdentity (&id._size,	// [out] Pointer to the count of bytes in the entry
												// identifier pointed to by the lppEntryID parameter.
								&id._id);		// [out] Pointer to a pointer to the entry identifier
												// of the object providing the primary identity.
	}
	catch (...)
	{
		Mapi::HandleGPF ("IMAPISession::QueryIdentity");
		throw;
	}
}
