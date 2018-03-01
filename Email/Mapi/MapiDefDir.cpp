//-----------------------------------------------------
//  MapiDefDir.cpp
//  (c) Reliable Software 2001 -- 2004
//-----------------------------------------------------

#include "precompiled.h"
#include "MapiDefDir.h"
#include "MapiAddrBook.h"
#include "MapiUser.h"
#include "MapiBuffer.h"

using namespace Mapi;

DefaultDirectory::DefaultDirectory (Session & session, bool browseOnly)
	: _session (session),
	  _storesTable (_session),
	  _msgStore (_session, _storesTable)
{}

void DefaultDirectory::GetStatusTable (Interface<IMAPITable> & table)
{
	_session.GetStatusTable (table);
}

void DefaultDirectory::OpenAddressBook (AddressBook & addrBook)
{
	_session.OpenAddressBook (addrBook.Get ());
}

// Return true if mail user opened successfuly
bool DefaultDirectory::OpenMailUser (EntryId const & id, MailUser & user)
{
	ObjectType objType;
	Result result = _session.OpenEntry (id, user, objType);
	return result.IsOk () && objType.IsMailUser ();
}

void DefaultDirectory::QueryIdentity (PrimaryIdentityId & id)
{
	Result result = _session.QueryIdentity (id);
	id.SetValid (result.IsOk () && !result.IsWarningNoService ());
}
