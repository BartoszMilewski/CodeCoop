//-----------------------------------
//  (c) Reliable Software 1997 - 2008
//-----------------------------------

#include "precompiled.h"
#include "FileState.h"

#include <Dbg/Assert.h>

FileState::StateName const FileState::_name [] =
{
// State bits (2 means any value):
//     P  Ps Pso O  co R  ro S  so coDel soDel M  Mc
    { {0, 0, 0,  0, 0, 0, 0, 0, 0, 0,    0,    0, 0},	"----"						},
    { {1, 0, 0,  0, 0, 0, 0, 0, 0, 0,    0,    0, 0},	"In"						},
    { {1, 2, 2,  1, 1, 2, 2, 2, 2, 0,    0,    0, 0},	"Out"						},
    { {1, 2, 2,  0, 1, 0, 2, 0, 2, 0,    0,    0, 0},	"New"						},
    { {0, 0, 0,  1, 1, 0, 0, 0, 0, 1,    0,    0, 0},	"Deleted"					},
    { {0, 0, 0,  1, 1, 0, 0, 0, 0, 0,    0,    0, 0},	"Uncontrolled"				},
    { {1, 2, 1,  0, 0, 0, 0, 1, 1, 0,    0,    0, 0},	"Sync"						},
    { {0, 2, 1,  0, 0, 2, 2, 0, 1, 0,    1,    0, 0},	"Sync Delete"				},
    { {0, 2, 1,  0, 0, 2, 2, 0, 1, 0,    0,    0, 0},	"Sync Uncontrolled"			},
    { {1, 2, 1,  2, 1, 2, 1, 1, 1, 0,    0,    1, 0},	"Merge"						},
    { {1, 2, 1,  2, 1, 2, 1, 1, 1, 0,    0,    1, 1},	"Merge Conflict"			},
	{ {0, 2, 1,  1, 1, 1, 1, 1, 1, 1,    0,    1, 0},	"Merge Local Delete",		},
	{ {0, 2, 1,  1, 1, 1, 1, 1, 1, 0,    0,    1, 0},	"Merge Local Uncontrolled",	},
	{ {1, 1, 1,  0, 1, 1, 1, 0, 1, 0,    1,    1, 0},	"Merge Sync Delete",		},
	{ {1, 1, 1,  0, 1, 1, 1, 0, 1, 0,    0,    1, 0},	"Merge Sync Uncontrolled",	},
    { {1, 1, 1,  1, 1, 1, 1, 1, 1, 1,    1,    1, 1},	0							}
};

char const * FileState::GetName () const
{
	for (int i = 0; _name [i].str != 0; ++i)
	{
        if (IsBitEqual (_bits._P       , _name [i].bits [0])  &&
			IsBitEqual (_bits._PS      , _name [i].bits [1])  &&
			IsBitEqual (_bits._pso     , _name [i].bits [2])  &&
            IsBitEqual (_bits._O       , _name [i].bits [3])  &&
            IsBitEqual (_bits._co      , _name [i].bits [4])  &&
            IsBitEqual (_bits._R       , _name [i].bits [5])  &&
			IsBitEqual (_bits._ro      , _name [i].bits [6])  &&
            IsBitEqual (_bits._S       , _name [i].bits [7])  &&
            IsBitEqual (_bits._so      , _name [i].bits [8])  &&
            IsBitEqual (_bits._coDelete, _name [i].bits [9])  &&
            IsBitEqual (_bits._soDelete, _name [i].bits [10]) &&
			IsBitEqual (_bits._M       , _name [i].bits [11]) &&
			IsBitEqual (_bits._Mc      , _name [i].bits [12]))
		{
			return _name [i].str;
		}
	}
	return "Transitory";	// Well we have problems, but why to scary the user
}

void FileState::Serialize (Serializer & out) const
{
    out.PutLong (_value);
}

void FileState::Deserialize (Deserializer & in, int version)
{
	_value = in.GetLong ();
	// Clear volatile bits
	_bits._coDiff = 0;
	_bits._soDiff = 0;
	_bits._Rnc = 0;
	_bits._Rn = 0;
	_bits._Mv = 0;
	_bits._Out = 0;
}

bool FileState::IsDirtyUncontrolled () const
{
	if (IsPresentIn (Area::Project))
		return false;

	// Not controlled
	if (_value == 0)
		return false;	// Correct state

	// Not controlled but state is not none - check why.
	if (IsRelevantIn (Area::Original) || IsRelevantIn (Area::Synch))
		return false;	// Checked out or synched out.

	return true;
}

bool FileState::IsPresentIn (Area::Location loc) const
{
	switch (loc)
	{
	case Area::Project:
		return _bits._P != 0;
		break;
    case Area::Original:		// Original Area *.og1 or *.og2
		return _bits._O != 0;
		break;
    case Area::Reference:		// Reference Area *.ref
		return _bits._R != 0;
		break;
    case Area::Synch:			// Synch Area *.syn
		return _bits._S != 0;
		break;
    case Area::PreSynch:		// Project PreSynch Area used during script unpack *.bak
		return _bits._PS != 0;
		break;
    case Area::Staging:			// Project Staging Area *.prj
		return false;
		break;
    case Area::OriginalBackup:	// OriginalBackupArea *.ogx where x is always previous original id
	case Area::Temporary:		// Temporary area *.tmp used for reconstructing files
	case Area::Compare:			// Compare area *.cmp used for comparing two reconstructed versions
	case Area::LocalEdits:		// Preserved local edits
		return false;
		break;
	default:
		Assert (!"Bad area location");
	}
	return false;
}

bool FileState::IsRelevantIn (Area::Location loc) const
{
	switch (loc)
	{
	case Area::Project:
		return true;
		break;
    case Area::Original:    // Original Area *.og1 or *.og2
		return _bits._co != 0;
		break;
    case Area::Reference:   // Reference Area *.ref
		return _bits._ro != 0;
		break;
    case Area::Synch:       // Synch Area *.syn
		return _bits._so != 0;
		break;
    case Area::PreSynch:	// Project PreSynch Area used during script unpack *.bak
		return _bits._pso != 0;
		break;
    case Area::Staging:		// Project Staging Area *.prj
		return false;
		break;
    case Area::OriginalBackup:// OriginalBackupArea *.ogx where x is always previous original id
	case Area::Temporary:	// Temporary area *.tmp used for reconstructing files
	case Area::Compare:		// Compare area *.cmp used for comparing two reconstructed versions
	case Area::LocalEdits:	// Preserved local edits
		return false;
		break;
	default:
		Assert (!"Bad area location");
	}
	return false;
}

void FileState::SetPresentIn (Area::Location loc, bool val)
{
	switch (loc)
	{
	case Area::Project:
		_bits._P = val;
		break;
    case Area::Original:    // Original Area *.og1 or *.og2
		_bits._O  = val;
		break;
    case Area::Reference:   // Reference Area *.ref
		_bits._R  = val;
		break;
    case Area::Synch:       // Synch Area *.syn
		_bits._S  = val;
		break;
    case Area::PreSynch:	// Project PreSynch Area used during script unpack *.bak
		_bits._PS  = val;
		break;
	default:
		Assert (!"Bad area location");
	}
}

void FileState::SetRelevantIn (Area::Location loc, bool val)
{
	switch (loc)
	{
    case Area::Original:    // Original Area *.og1 or *.og2
		_bits._co  = val;
		break;
    case Area::Reference:   // Reference Area *.ref
		_bits._ro  = val;
		break;
    case Area::Synch:       // Synch Area *.syn
		_bits._so  = val;
		break;
    case Area::PreSynch:	// Project PreSynch Area used during script unpack *.bak
		_bits._pso  = val;
		break;
	default:
		Assert (!"Bad area location");
	}
}

std::ostream & operator<<(std::ostream & os, FileState fs)
{
	os << "P:" << fs.IsPresentIn (Area::Project) ? 1 : 0;
	os << " Ps:" << fs.IsPresentIn (Area::PreSynch) ? 1 : 0;
	os << " Pso:" << fs.IsRelevantIn (Area::PreSynch) ? 1 : 0;
	os << " O:" << fs.IsPresentIn (Area::Original) ? 1 : 0;
	os << " co:" << fs.IsRelevantIn (Area::Original) ? 1 : 0;
	os << " R:" << fs.IsPresentIn (Area::Reference) ? 1 : 0;
	os << " ro:" << fs.IsRelevantIn (Area::Reference) ? 1 : 0;
	os << " S:" << fs.IsPresentIn (Area::Synch) ? 1 : 0;
	os << " so:" << fs.IsRelevantIn (Area::Synch) ? 1 : 0;
	os << " coDel:" << fs.IsCoDelete () ? 1 : 0;
	os << " soDel:" << fs.IsSoDelete () ? 1 : 0;
	os << " M:" << fs.IsMerge () ? 1 : 0;
	os << " Mc:" << fs.IsMergeConflict () ? 1 : 0;
	os << " coDiff: " << fs.IsCoDiff () ? 1 : 0;
	os << " soDiff: " << fs.IsSoDiff () ? 1 : 0;
	os << " Rnc: " << fs.IsResolvedNameConflict () ? 1 : 0;
	os << " Rn: " << fs.IsRenamed () ? 1 : 0;
	os << " Mv: " << fs.IsMoved () ? 1 : 0;
	os << " TypeCh: " << fs.IsTypeChanged ()? 1 : 0;
	os << " Out: " << fs.IsCheckedOutByOthers () ? 1 : 0;
	os << " Value: 0x" << std::hex << fs.GetValue () << std::dec;
	return os;
}
