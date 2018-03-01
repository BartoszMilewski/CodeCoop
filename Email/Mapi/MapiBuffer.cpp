//
// (c) Reliable Software 1998 -- 2002
//

#include "precompiled.h"
#include "MapiBuffer.h"
#include "MapiSession.h"
#include "MapiStore.h"
#include "MapiMessage.h"
#include "MapiEx.h"

#include <Dbg/Assert.h>

#include <mapival.h>

#if !defined (NDEBUG)
#include <LightString.h>

void DumpProps (int count, SPropValue const * props, Msg & info);
#endif

using namespace Mapi;

//
// Mapi SRowSet
//

RowSet::RowSet (RowSet & buf)
{
	if (_rows != buf._rows)
	{
		Free ();
		_rows = buf.Release ();
	}
}

RowSet::~RowSet ()
{
	Free ();
}

SBinary const * RowSet::GetEntryId (unsigned int i) const
{
	Assert (i < _rows->cRows);
	for (unsigned int k = 0; k < _rows->aRow [i].cValues; k++)
	{
		if (_rows->aRow [i].lpProps [k].ulPropTag == PR_ENTRYID)
		{
			return &_rows->aRow [i].lpProps [k].Value.bin;
		}
	}
	Assert ("ENTRYID property not found in the query row set");
	return 0;
}

char const * RowSet::GetDisplayName (unsigned int i) const
{
	Assert (i < _rows->cRows);
	for (unsigned int k = 0; k < _rows->aRow [i].cValues; k++)
	{
		if (_rows->aRow [i].lpProps [k].ulPropTag == PR_DISPLAY_NAME)
		{
			return _rows->aRow [i].lpProps [k].Value.lpszA;
		}
	}
	Assert ("DISPLAY_NAME property not found in the query row set");
	return 0;
}

unsigned long RowSet::GetAttachNum (unsigned int i) const
{
	Assert (i < _rows->cRows);
	for (unsigned int k = 0; k < _rows->aRow [i].cValues; k++)
	{
		if (_rows->aRow [i].lpProps [k].ulPropTag == PR_ATTACH_NUM)
		{
			return _rows->aRow [i].lpProps [k].Value.ul;
		}
	}
	Assert ("ATTACH_NUM property not found in the query row set");
	return 0;
}

bool RowSet::IsFolder (unsigned int i) const
{
	Assert (i < _rows->cRows);
	for (unsigned int k = 0; k < _rows->aRow [i].cValues; k++)
	{
		if (_rows->aRow [i].lpProps [k].ulPropTag == PR_OBJECT_TYPE)
		{
			return _rows->aRow [i].lpProps [k].Value.ul == MAPI_FOLDER;
		}
	}
	Assert ("OBJECT_TYPE property not found in the query row set");
	return false;
}

RowSet & RowSet::operator = (RowSet & buf)
{
	if (_rows != buf._rows)
	{
		Free ();
		_rows = buf.Release ();
	}
	return *this;
}

void RowSet::Dump (char const * title) const
{
#if !defined (NDEBUG)
	Msg info;
	info << _rows->cRows << " rows:\n";
	for (unsigned int i = 0; i < _rows->cRows; i++)
	{
		info << _rows->aRow [i].cValues << " properties at address 0x" << std::hex << _rows->aRow [i].lpProps << "\n";
		DumpProps (_rows->aRow [i].cValues, _rows->aRow [i].lpProps, info);
	}
	MessageBox (0, info.c_str (), title, MB_ICONEXCLAMATION | MB_OK);
#endif
}

LPSRowSet RowSet::Release ()
{
	LPSRowSet tmp = _rows;
	_rows = 0;
	return tmp;
}

void RowSet::Free ()
{
	if (_rows != 0)
	{
		::FreeProws (_rows);
		_rows = 0;
	}
}

//
// Entry Ids
//

ReceiveFolderId::ReceiveFolderId (MsgStore & msgStore)
{
	msgStore.GetReceiveFolderId (*this);
}

void FolderEntryId::Init (MsgStore & msgStore, unsigned long folderIdTag)
{
	msgStore.GetFolderId (*this, folderIdTag);
	Assert (!FBadProp (_p));
}

MsgEntryId::MsgEntryId (Message & msg)
{
	msg.GetId (*this);
	Assert (!FBadProp (_p));
}
