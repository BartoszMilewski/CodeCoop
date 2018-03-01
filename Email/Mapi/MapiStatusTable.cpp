//---------------------------------------------
// (c) Reliable Software 2004
//---------------------------------------------

#include "precompiled.h"
#include "MapiStatusTable.h"
#include "MapiDefDir.h"
#include "MapiProperty.h"
#include "MapiRestriction.h"
#include "MapiGPF.h"

using namespace Mapi;

StatusTable::StatusTable (DefaultDirectory & defDir)
{
	defDir.GetStatusTable (_table);
}

std::string StatusTable::GetLoggedProfileName ()
{
	PropertyList columns;
	columns.Add (PR_DISPLAY_NAME);
	columns.Add (PR_RESOURCE_TYPE);
	LongPropertyFilter filter (PR_RESOURCE_TYPE, RELOP_EQ, MAPI_SUBSYSTEM);
	Assert (!FBadRestriction (&filter));
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
		Mapi::HandleGPF ("::HrQueryAllRows (IMAPITable)");
		throw;
	}
	_table.ThrowIfError (result, "MAPI -- Cannot retrieve profile name.");
	Assert (!::FBadRowSet (resultRows.Get ()));
	if (resultRows.Count () != 0)
		return std::string (resultRows.GetDisplayName (0));
	else
		return std::string ();
}
