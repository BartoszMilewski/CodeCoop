//-----------------------------------------
// (c) Reliable Software 1998-2002
// ----------------------------------------
#include "precompiled.h"
#include "RecordSets.h"
#include "RecipKey.h"
#include "UserIdPack.h"
#include "resource.h"

#include <Dbg/Assert.h>
#include <StringOp.h>

int UnameRecordSet::GetRow (Bookmark const & bookmark) const
{
	for (unsigned int i = 0; i < _names.size (); ++i)
	{
		if (bookmark.GetName () == _names [i])
			return i;
	}
	return -1;
}

std::string const & UnameRecordSet::GetName (unsigned int row) const
{
	Assert (row < _names.size ());
	return _names [row];
}

// ----------------------------------

char const * PublicInboxRecordSet::_columnHeadings [] =
{
	"Comment", "Direction", "Status"
};

PublicInboxRecordSet::PublicInboxRecordSet (Table & t)
{
	t.QueryUniqueNames (_names);
	_comments.reserve  (_names.size ());
	_statuses.reserve  (_names.size ());

	for (unsigned int i = 0; i < _names.size (); ++i)
	{
		_comments.push_back (t.GetStringField (Table::colComment, _names [i]));
		if (_comments.back ().empty ())
			_comments.back () = "No comment available";
		_statuses.push_back (static_cast<ScriptStatus> (
			t.GetNumericField (Table::colStatus, _names [i])));
	}
}

unsigned int PublicInboxRecordSet::GetColCount () const
{
    return sizeof (_columnHeadings) / sizeof (_columnHeadings [0]); 
}

char const * PublicInboxRecordSet::GetColumnHeading (unsigned int col) const
{
	return _columnHeadings [col];
}

std::string PublicInboxRecordSet::GetFieldString (unsigned int row, unsigned int col) const
{
    // columns order:
    // Comment, Dispatch Status, Direction
    Assert (col < GetColCount ());
    Assert (row < GetRowCount ());

	switch (col)
	{
	case 0: // comment
		return _comments [row];
	case 1: // direction
		return _statuses [row].GetDirectionStr ();
	case 2: // dispatch status
		return _statuses [row].ShortDescription ();
	default:
		Assert (!"Bad column number");
		return "Bug";
	};
}

int PublicInboxRecordSet::CmpRows (unsigned int row1, unsigned int row2, int col) const
{
	Assert (row1 < _names.size ());
	Assert (row2 < _names.size ());

	if (row1 == row2)
		return 0;

	switch (col)
	{
	case 0: // comment
		return NocaseCompare (_comments [row1], _comments [row2]);
	case 1: // direction
		return CompareInts (_statuses [row1].GetDirection (), 
							_statuses [row2].GetDirection ());
	case 2: // status (no sorting)
		return 0;
	default:
		Assert (!"Invalid column number");
		return 0;
	};
}

void PublicInboxRecordSet::GetImage (unsigned int item, int & imageId, int & overlay) const
{
    overlay = 0;
	imageId = 0;
}

void PublicInboxRecordSet::GetItemIcons (std::vector<int> & iconResIds) const
{
	iconResIds.push_back (ID_ENVELOPE);
}

// ----------------------------------

char const * MemberRecordSet::_columnHeadings [] =
{
	"Project", "Members"
};

MemberRecordSet::MemberRecordSet (Table & t)
{
	t.QueryUniqueNames (_names);
	_ids.reserve  (_names.size ());

	for (unsigned int i = 0; i < _names.size (); ++i)
	{
		_ids.push_back (t.GetNumericField (Table::colMembers, _names [i]));
	}
}

unsigned int MemberRecordSet::GetColCount () const
{
    return sizeof (_columnHeadings) / sizeof (_columnHeadings [0]); 
}

char const * MemberRecordSet::GetColumnHeading (unsigned int col) const
{
	return _columnHeadings [col];
}

std::string MemberRecordSet::GetFieldString (unsigned int row, unsigned int col) const
{
    // columns order:
    // Project, Members
    Assert (col < GetColCount ());
    Assert (row < GetRowCount ());

	switch (col)
	{
	case 0: // project
		return _names [row];
	case 1: // members
		return ToString (_ids [row]);
	default:
		Assert (!"Bad column number");
		return "Bug";
	};
}

int MemberRecordSet::CmpRows (unsigned int row1, unsigned int row2, int col) const
{
	Assert (row1 < _names.size ());
	Assert (row2 < _names.size ());

	if (row1 == row2)
		return 0;

	switch (col)
	{
	case 0: // project name
		return NocaseCompare (_names [row1], _names [row2]);
	case 1: // member count (descending order)
		return - CompareInts (_ids [row1], _ids [row2]);
	default:
		Assert (!"Invalid column number");
		return 0;
	};
}

void MemberRecordSet::GetImage (unsigned int item, int & imageId, int & overlay) const
{
    overlay = 0;
	imageId = 0;
}

void MemberRecordSet::GetItemIcons (std::vector<int> & iconResIds) const
{
	iconResIds.push_back (ID_USERS);
}

// ----------------------------------
std::string const ProjectMemberRecordSet::_stActive  = "Active";
std::string const ProjectMemberRecordSet::_stRemoved = "Removed";
std::string const ProjectMemberRecordSet::_locLocal  = "Here";
std::string const ProjectMemberRecordSet::_locCluster= "Satellite";

char const * ProjectMemberRecordSet::_columnHeadings [] =
{
	// Revisit: Hub Id column no longer needed
	"User ID", "Local Hub", "Local Transport", "Status", "Location"
};

const int ProjectMemberRecordSet::_itemIconIds [] =
{
    ID_USER,
    ID_EXUSER,
    ID_SATUSER,
    ID_EXSATUSER
};

ProjectMemberRecordSet::ProjectMemberRecordSet (Table & t, Restriction const & restrict)
  : _projectName (restrict.GetString ())
{
	t.QueryUniqueTripleKeys (_keys, &restrict);
	_ids.reserve (_keys.size ());
	_paths.reserve (_keys.size ());
	_userIds.reserve (_keys.size ());
	_intUserIds.reserve (_keys.size ());
	
	for (unsigned int i = 0; i < _keys.size (); ++i)
	{
		RecipientKey const & recip = static_cast<RecipientKey const &>(_keys [i]);
		UserIdPack loc (recip.GetUserId ());
		_userIds.push_back (loc.GetUserIdString ());
		_intUserIds.push_back (loc.GetUserId ());

		_paths.push_back  (t.GetStringField  (Table::colPath,   _keys [i]));
		_ids.push_back	  (t.GetNumericField (Table::colStatus, _keys [i]));
	}
}

int ProjectMemberRecordSet::GetRow (Bookmark const & bookmark) const
{
	for (unsigned int i = 0; i < _keys.size (); ++i)
	{
		if (bookmark.GetKey ().IsEqual (_keys [i]))
			return i;
	}
	return -1;
}

TripleKey const & ProjectMemberRecordSet::GetKey (unsigned int row) const
{
	Assert (row < _keys.size ());
	return _keys [row];
}

int ProjectMemberRecordSet::CmpRows (unsigned int row1, unsigned int row2, int col) const
{
	Assert (row1 < _ids.size ());
	Assert (row2 < _ids.size ());

	if (row1 == row2)
		return 0;

	RecipientKey const & recip1 = static_cast<RecipientKey const &>(_keys [row1]);
	RecipientKey const & recip2 = static_cast<RecipientKey const &>(_keys [row2]);

	switch (col)
	{
	case 0: // user id
		return CompareInts (_intUserIds [row1], _intUserIds [row2]);
	case 1: // Hub
		return NocaseCompare (recip1.GetHubId (), recip2.GetHubId ());
	case 2: // fwd path
		return NocaseCompare (_paths [row1], _paths [row2]);
	case 3: // status
		return CompareInts (_ids [row1], _ids [row2]);
	case 4: // location (hub/sat)
		return - CompareInts (recip1.IsLocal (), recip2.IsLocal ());
	default:
		Assert (!"Invalid column number");
		return 0;
	};
}

unsigned int ProjectMemberRecordSet::GetColCount () const
{
    return sizeof (_columnHeadings) / sizeof (_columnHeadings [0]); 
}

char const * ProjectMemberRecordSet::GetColumnHeading (unsigned int col) const
{
	if (col == 0 && _projectName.empty ())
		return "Computer Name";

	return _columnHeadings [col];
}

std::string ProjectMemberRecordSet::GetFieldString (unsigned int row, unsigned int col) const
{
    // columns order:
    // HubId, User id label, Forwarding path
    Assert (col < GetColCount ());
    Assert (row < GetRowCount ());

	RecipientKey const & recip = static_cast<RecipientKey const &>(_keys [row]);
	switch (col)
	{
	case 0: // user id
		return _userIds [row];
		break;
	case 1: // Hub
		return recip.GetHubId ();
		break;
	case 2: // forwarding path
		return _paths [row];
		break;
	case 3: // recipient status
		return _ids [row] ? _stActive : _stRemoved;
		break;
	case 4: // recipient location (local/cluster)
		return recip.IsLocal () ? _locLocal : _locCluster;
	default:
		Assert (!"Bad column number");
		return "Bug";
	};
}

void ProjectMemberRecordSet::GetImage (unsigned int item, int & imageId, int & overlay) const
{
    overlay = 0;
	RecipientKey const & recip = static_cast<RecipientKey const &>(_keys [item]);
    if (_ids [item])
	{
		// active
		imageId = recip.IsLocal () ? iUser : iSatUser;
	}
    else
	{
		// removed
		imageId = recip.IsLocal () ? iExUser : iExSatUser;
	}
}

void ProjectMemberRecordSet::GetItemIcons (std::vector<int> & iconResIds) const
{
    const int iconCount = sizeof (_itemIconIds) / sizeof (_itemIconIds [0]);
    iconResIds.reserve (iconCount);
    for (int i = 0; i < iconCount; ++i)
		iconResIds.push_back (_itemIconIds [i]);
}

// ------------------------------------------------
char const * RemoteHubRecordSet::_columnHeadings [] =
{
	"Hub's Email/Name", "Transport", "Type"
};

const int RemoteHubRecordSet::_itemIconIds [] =
{
    ID_REMOTE_HUB
};

char const * RemoteHubRecordSet::_methodStr [] =
{
	"Unknown",
	"LAN",
	"Email",
};

RemoteHubRecordSet::RemoteHubRecordSet (Table & t)
{
	t.QueryUniqueNames (_names);
	_routes.reserve (_names.size ());
	_methods.reserve (_names.size ());
	
	for (unsigned int i = 0; i < _names.size (); ++i)
	{
		_routes.push_back  (t.GetStringField  (Table::colRoute,  _names [i]));
		_methods.push_back (t.GetNumericField (Table::colMethod, _names [i]));
	}
}

int RemoteHubRecordSet::CmpRows (unsigned int row1, unsigned int row2, int col) const
{
	Assert (row1 < _names.size ());
	Assert (row2 < _names.size ());

	if (row1 == row2)
		return 0;

	switch (col)
	{
	case 0: // hub Id
		return NocaseCompare (_names [row1], _names [row2]);
	case 1: // route
		return NocaseCompare (_routes [row1], _routes [row2]);
	case 2: // method
		return CompareInts (_methods [row1], _methods [row2]);
	default:
		Assert (!"Invalid column number");
		return 0;
	};
}

unsigned int RemoteHubRecordSet::GetColCount () const
{
    return sizeof (_columnHeadings) / sizeof (_columnHeadings [0]); 
}

char const * RemoteHubRecordSet::GetColumnHeading (unsigned int col) const
{
	return _columnHeadings [col];
}

std::string RemoteHubRecordSet::GetFieldString (unsigned int row, unsigned int col) const
{
    // columns order:
    // Hub Id, Route, Transport Method
    Assert (col < GetColCount ());
    Assert (row < GetRowCount ());

	switch (col)
	{
	case 0: // hub id
		return _names [row];
	case 1: // route
		return _routes [row];
	case 2: // transport method
		return _methodStr [_methods [row]];
	default:
		Assert (!"Bad column number");
		return "Bug";
	};
}

void RemoteHubRecordSet::GetImage (unsigned int item, int & imageId, int & overlay) const
{
	imageId = 0;
    overlay = 0;
}

void RemoteHubRecordSet::GetItemIcons (std::vector<int> & iconResIds) const
{
    const int iconCount = sizeof (_itemIconIds) / sizeof (_itemIconIds [0]);
    iconResIds.reserve (iconCount);
    for (int i = 0; i < iconCount; ++i)
		iconResIds.push_back (_itemIconIds [i]);
}

// Quarantine

char const * QuarantineRecordSet::_columnHeadings [] =
{
	"Script", "Status"
};

QuarantineRecordSet::QuarantineRecordSet (Table & t)
{
	t.QueryUniqueNames (_names);
	_statuses.reserve (_names.size ());

	for (unsigned int i = 0; i < _names.size (); ++i)
	{
		_statuses.push_back (t.GetStringField (Table::colComment, _names [i]));
	}
}

int QuarantineRecordSet::CmpRows (unsigned int row1, unsigned int row2, int col) const
{
	Assert (row1 < _names.size ());
	Assert (row2 < _names.size ());

	if (row1 == row2)
		return 0;

	if (col == 0)
		return NocaseCompare (_names [row1], _names [row2]);
	else
		return NocaseCompare (_statuses [row1], _statuses [row2]);
}

unsigned int QuarantineRecordSet::GetColCount () const
{
	return sizeof (_columnHeadings) / sizeof (_columnHeadings [0]); 
}

char const * QuarantineRecordSet::GetColumnHeading (unsigned int col) const
{
	return _columnHeadings [col];
}

std::string QuarantineRecordSet::GetFieldString (unsigned int row, unsigned int col) const
{
	Assert (col < GetColCount ());
	Assert (row < GetRowCount ());

	if (col == 0) // script filename
		return _names [row];
	else
		return _statuses [row]; 
}

void QuarantineRecordSet::GetImage (unsigned int item, int & imageId, int & overlay) const
{
	imageId = 0;
	overlay = 0;
}

void QuarantineRecordSet::GetItemIcons (std::vector<int> & iconResIds) const
{
	iconResIds.push_back (ID_ENVELOPE);
}

// AlertLog

char const * AlertLogRecordSet::_columnHeadings [] =
{
	"Last Occurrence", "Count", "Alert"
};

AlertLogRecordSet::AlertLogRecordSet (Table & t)
{
	t.QueryUniqueIds (_ids);
	_dates.reserve  (_ids.size ());
	_counts.reserve (_ids.size ());

	for (unsigned int i = 0; i < _ids.size (); ++i)
	{
		_names.push_back (t.GetStringField (Table::colComment, _ids [i]));
		_dates.push_back (t.GetStringField (Table::colDate, _ids [i]));
		_counts.push_back (t.GetNumericField (Table::colCount, _ids [i]));
	}
}

int AlertLogRecordSet::CmpRows (unsigned int row1, unsigned int row2, int col) const
{
	Assert (row1 < _ids.size ());
	Assert (row2 < _ids.size ());

	return row1 - row2; // no sorting
}

unsigned int AlertLogRecordSet::GetColCount () const
{
	return sizeof (_columnHeadings) / sizeof (_columnHeadings [0]); 
}

char const * AlertLogRecordSet::GetColumnHeading (unsigned int col) const
{
	return _columnHeadings [col];
}

std::string AlertLogRecordSet::GetFieldString (unsigned int row, unsigned int col) const
{
	Assert (col < GetColCount ());
	Assert (row < GetRowCount ());
	if (col == 0) // Date
		return _dates [row];
	else if (col == 1) // Count
		return ToString (_counts [row]);
	else // Alert
		return _names [row];
}

void AlertLogRecordSet::GetImage (unsigned int item, int & imageId, int & overlay) const
{
	imageId = 0;
	overlay = 0;
}

void AlertLogRecordSet::GetItemIcons (std::vector<int> & iconResIds) const
{
	iconResIds.push_back (ID_ALERT_LOG);
}

int AlertLogRecordSet::GetRow (Bookmark const & bookmark) const
{
	for (unsigned int i = 0; i < _ids.size (); ++i)
	{
		if (bookmark.GetId () == _ids [i])
			return i;
	}
	return -1;
}
