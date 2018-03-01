#if !defined (MAPISTORE_H)
#define MAPISTORE_H
//
// (c) Reliable Software 1998
//

#include "MapiRestriction.h"
#include "MapiBuffer.h"

#include <Com\Shell.h>

class Session;
class OutgoingMessage;
class RecipientList;

class MsgStoresTable : public SIfacePtr<IMAPITable>
{
public:
	MsgStoresTable (Session & session);

	operator struct IMAPITable * () { return _p; }
};

class DefaultMsgStoreID
{
public:
	DefaultMsgStoreID (MsgStoresTable & storesTable);
	~DefaultMsgStoreID ();

	bool Found () const;
	
	SBinary const & GetEntryID () const { return _resultRows->aRow [0].lpProps [0].Value.bin; }

private:
	LPSRowSet			_resultRows;
};

class MsgStore : public SIfacePtr<IMsgStore>
{
public:
	MsgStore (Session & session, SBinary const & storeID);
	bool IsOutboxFolderValid ();
	void GetProperty (MapiBuffer<SPropValue> & prop, ULONG propTag);
	operator struct IMsgStore * () { return _p; }
};

class Outbox : public SIfacePtr<IMAPIFolder>
{
public:
	Outbox (Session & session);

	void Submit (OutgoingMessage const & msg, RecipientList const & recipients, bool verbose);
};

class MapiFolder : public SIfacePtr<IMAPIFolder>
{
public:
	MapiFolder (MsgStore & store);
	MapiFolder (MsgStore & store, SBinary const * folderID);
};

class HierarchyTable : public SIfacePtr<IMAPITable>
{
public:
	HierarchyTable (MapiFolder & folder);
	void Dump (MsgStore & store, char const * title);
};

class ContentsTable : public SIfacePtr<IMAPITable>
{
public:
	ContentsTable (MapiFolder & folder);
	void Dump (char const * title);
};

#endif
