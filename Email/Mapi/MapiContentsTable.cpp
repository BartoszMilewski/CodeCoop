//-----------------------------------------------------
//  MapiContentsTable.cpp
//  (c) Reliable Software 2001 -- 2002
//-----------------------------------------------------

#include "precompiled.h"
#include "MapiContentsTable.h"
#include "MapiIface.h"
#include "MapiStore.h"
#include "MapiBuffer.h"
#include "MapiProperty.h"
#include "MapiRestriction.h"
#include "MapiIface.h"
#include <Dbg/Out.h>

namespace Mapi
{

ContentsTable::ContentsTable (Folder & folder, bool unreadOnly)
{
	dbg << "Mapi Contents Table" << std::endl;
	Interface<IMAPITable> table;
	folder.GetContentsTable (table);
	// We are interested only in the entry id column
	PropertyList columns;
	columns.Add (PR_ENTRYID);
	Result tableResult = table->SetColumns (columns.Cnv2TagArray (), 0);
	table.ThrowIfError (tableResult, "MAPI -- Cannot limit contents table column set.");
	if (unreadOnly)
	{
		// Restrict only to unread messages
		IsUnreadMessage isUnread;
		tableResult = table->Restrict (&isUnread, 0);
		// If limiting contents table to unread messages is to complex
		// for the MAPI we will look at all read/unread messages
		if (!tableResult.IsTooComplex ())
			table.ThrowIfError (tableResult, "MAPI -- Cannot restrict contents table to unread messages only.");
	}
	// Table can have a lot of rows. Read them in groups and convert entry ids to strings.
	RowSet rows;
	bool keepGoing = true;
	while (keepGoing)
	{
		tableResult = table->QueryRows (RowsPerQuery, 0, &rows);
		table.ThrowIfError (tableResult, "MAPI -- Cannot query contents table rows.");
		keepGoing = rows.Count () != 0;
		if (keepGoing)
		{
			Assert (!FBadRowSet (rows.Get ()));
			for (unsigned int i = 0; i < rows.Count (); ++i)
			{
				SBinary const * entryId = rows.GetEntryId (i);
				std::vector<unsigned char> id;
				id.resize (entryId->cb);
				memcpy (&id [0], entryId->lpb, entryId->cb);
				_ids.push_back (id);
			}
			::FreeProws (rows.Release ());
		}
	}
	dbg << "Mapi Contents Table, done" << std::endl;
}

}