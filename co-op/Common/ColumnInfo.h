#if !defined (COLUMNINFO_H)
#define COLUMNINFO_H
//------------------------------------
//  (c) Reliable Software, 2005 - 2006
//------------------------------------

#include "Table.h"
#include <Ctrl/ListView.h>

namespace Column
{
	struct Info
	{
		const char *				heading;
		Win::Report::ColAlignment	alignment;
		SortType					sortOrder;
		unsigned int				defaultWidth;	// In chars
	};

	extern const Info BrowsingFiles [];
	extern const Info MiniProject [];
	extern const Info Checkin [];
	extern const Info Synch [];
	extern const Info Mailbox [];
	extern const Info History [];
	extern const Info Project [];
	extern const Info NotInProject [];
	extern const Info ScriptDetails [];
	extern const Info MergeDetails [];
}

#endif
