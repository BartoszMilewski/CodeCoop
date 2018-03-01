//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "FileData.h"
#include "GlobalId.h"
#include "FileChanges.h"

#include <Dbg/Out.h>

void AreaFileData::Serialize (Serializer& out) const
{
	_uname.Serialize (out);
    out.PutLong (_location);
	out.PutLong (_type.GetValue ());
}

void AreaFileData::Deserialize (Deserializer& in, int version)
{
	if (version < 31)
	{
		// in AreaFileData version < 31 no stored types
		// _type will be patched by FileData Deserializer
		_uname.Deserialize (in, version);
		_location = static_cast<Area::Location> (in.GetLong ());
	}
	else
	{
		_uname.Deserialize (in, version);
		_location = static_cast<Area::Location> (in.GetLong ());
		_type = FileType (in.GetLong ());
	}
}

//
// FileData
//

// Functional
class AreaEq: public std::unary_function<AreaFileData, bool>
{
public:
	explicit AreaEq (Area::Location area): _area (area) {}
	bool operator () (AreaFileData const & uAName) const
	{
		return uAName.GetLocation () == _area;
	}
private:
	Area::Location	_area;
};

void FileData::Rename (UniqueName const & unameNew)
{
	Assert (!_type.IsRoot ());
	if (!_state.IsNew () && !_type.IsFolder ())
	{
		// Add current name as alias to all the areas
		// that have no alias of their own (except Project area)
		// Or, if they have the the New name as alias, remove it
		for (Area::Seq seq; !seq.AtEnd (); seq.Advance ())
		{
			Area::Location area = seq.GetArea ();
			if (_state.IsPresentIn (area))
			{
				// find alias for this area
				AliasIter it = std::find_if (_aliases.begin (), _aliases.end (), AreaEq (area));

				if (it == _aliases.end ()) // not found
				{
					if (area != Area::Project)
						AddUnameAlias (_uname, area);
				}
				else if (unameNew.IsStrictlyEqual (it->GetUniqueName ()) && _type.IsEqual (it->GetType ()))
				{
					Assert (area != Area::Project);
					//remove alias  
					_aliases.erase (it);
				}
			}
		}
	}
    _uname.Init (unameNew);
}

void FileData::ChangeType (FileType const & type)
{
	if (!_state.IsNew ())
	{
		for (Area::Seq seq; !seq.AtEnd (); seq.Advance ())
		{
			Area::Location area = seq.GetArea ();
			if (_state.IsPresentIn (area))
			{
				// find alias for this area
				AliasIter it = std::find_if (_aliases.begin (), _aliases.end (), AreaEq (area));

				if (it == _aliases.end ()) // not found
				{
					if (area != Area::Project)
						AddTypeAlias (_type, area);
				}
				else if (type.IsEqual (it->GetType ()) && _uname.IsEqual (it->GetUniqueName ()))
				{
					Assert (area != Area::Project);
					//remove alias  
					_aliases.erase (it);
				}
			}
		}
	}
    _type = type;
}


bool FileData::IsRenamedIn (Area::Location loc) const
{
	AliasIterC it = std::find_if (_aliases.begin (), _aliases.end (), AreaEq (loc));
	if (it == _aliases.end ())
		return false;

	UniqueName const & alias = it->GetUniqueName ();
	if (!alias.IsStrictlyEqual (_uname))
	{
		return true;
	}
	return false;
}

bool FileData::IsTypeChangeIn (Area::Location loc) const
{
	AliasIterC it = std::find_if (_aliases.begin (), _aliases.end (), AreaEq (loc));
	if (it == _aliases.end ())
		return false;

	FileType const & type = it->GetType ();
	if (!type.IsEqual (_type))
	{
		return true;
	}
	return false;
}


// Functional
class UnameEq: public std::unary_function<AreaFileData, bool>
{
public:
	explicit UnameEq (UniqueName const & uname): _uname (uname) {}
	bool operator () (AreaFileData const & fd) const
	{
		UniqueName const & alias = fd.GetUniqueName ();
		return alias.IsStrictlyEqual (_uname);
	}
private:
	UniqueName const & 	_uname;
};


bool FileData::IsRenamedAs (UniqueName const & uname) const
{
	AliasIterC it = std::find_if (_aliases.begin (), _aliases.end (), UnameEq (uname));
	return it != _aliases.end ();
}

UniqueName const & FileData::GetUnameIn (Area::Location loc) const
{
	AliasIterC it = std::find_if (_aliases.begin (), _aliases.end (), AreaEq (loc));
	// GetUnameIn must be called only after testing IsRenamed!
	Assert (it != _aliases.end ());
	return it->GetUniqueName ();
}

FileType const & FileData::GetTypeIn (Area::Location loc) const
{
	AliasIterC it = std::find_if (_aliases.begin (), _aliases.end (), AreaEq (loc));
	// GetFileTypeIn must be called only after testing IsChangeFileType!
	Assert (it != _aliases.end ());
	return it->GetType ();
}

void FileData::GetNonContentsFileChanges (FileChanges & file) const
{
	// Get rename/move information
	if (IsRenamedIn (Area::Original))
	{
		UniqueName const & alias = GetUnameIn (Area::Original);
		if (IsRenamedIn (Area::PreSynch))
		{
			// If alias in pre-synch area is identical to alias in the original
			// then we have synch rename only on checked out file
			UniqueName const & preSynchAlias = GetUnameIn (Area::PreSynch);
			if (!preSynchAlias.IsStrictlyEqual (alias))
			{
				if (alias.GetParentId () == _uname.GetParentId ())
					file.SetLocalRename (true);
				else
					file.SetLocalMove (true);
			}
		}
		else
		{
			if (alias.GetParentId () == _uname.GetParentId ())
				file.SetLocalRename (true);
			else
				file.SetLocalMove (true);
		}
	}
	else if (!_state.IsNew () && IsRenamedIn (Area::Reference))
	{
		// Historicaly file was localy renamed
		UniqueName const & alias = GetUnameIn (Area::Reference);
		if (alias.GetParentId () == _uname.GetParentId ())
			file.SetLocalRename (true);
		else
			file.SetLocalMove (true);
	}
	if (IsRenamedIn (Area::Synch) || IsRenamedIn (Area::PreSynch))
	{
		if (file.IsLocalMove () || file.IsLocalRename ())
		{
			// Check if alias in the Synch area is not identical to
			// the alias in the Reference area, because local rename
			// is copied to the Synch area when unpacking script that
			// only edits locally renamed file
			Assert (IsRenamedIn (Area::Reference));
			UniqueName const & synchAlias = GetUnameIn (Area::Synch);
			UniqueName const & referenceAlias = GetUnameIn (Area::Reference);
			if (!synchAlias.IsStrictlyEqual (referenceAlias))
			{
				if (synchAlias.GetParentId () == _uname.GetParentId ())
					file.SetSynchRename (true);
				else
					file.SetSynchMove (true);
			}
		}
		else
		{
			// No local renames/moves
			bool isRename = false;
			if (IsRenamedIn (Area::Synch))
			{
				UniqueName const & synchAlias = GetUnameIn (Area::Synch);
				isRename = synchAlias.GetParentId () == _uname.GetParentId ();
			}
			else
			{
				Assert (IsRenamedIn (Area::PreSynch));
				UniqueName const & preSynchAlias = GetUnameIn (Area::PreSynch);
				isRename = preSynchAlias.GetParentId () == _uname.GetParentId ();
			}
			if (isRename)
				file.SetSynchRename (true);
			else
				file.SetSynchMove (true);
		}
	}
	// Get file type change information
	file.SetLocalTypeChange (IsTypeChangeIn (Area::Original) || (!_state.IsNew () && IsTypeChangeIn (Area::Reference)));
	if (IsTypeChangeIn (Area::Synch))
	{
		if (file.IsLocalTypeChange ())
		{
			// Check if type alias in the Synch area is not identical to
			// the type alias in the Reference area, because local type change
			// is copied to the Synch area when unpacking script that
			// only edits file with local type change
			Assert (IsTypeChangeIn (Area::Reference));
			FileType synchType = GetTypeIn (Area::Synch);
			FileType referenceType = GetTypeIn (Area::Reference);
			file.SetSynchTypeChange (!synchType.IsEqual (referenceType));
		}
		else
		{
			// No local type changes
			file.SetSynchTypeChange (true);
		}
	}
	// Get delete/new information
	// Revisit: shoul we differentiate between delete and remove?
	file.SetLocalDelete (_state.IsToBeDeleted ());
	file.SetLocalNew (_state.IsNew ());
	file.SetSynchDelete (_state.IsSynchDelete ());
}

void FileData::AddUnameAlias (UniqueName const & uname, Area::Location loc)
{
	AliasIter it = std::find_if (_aliases.begin (), _aliases.end (), AreaEq (loc));
	if (it != _aliases.end ())
		it->SetUniqueName (uname);
	else
		_aliases.push_back (AreaFileData (uname, _type, loc));
}

void FileData::AddTypeAlias (FileType const & type, Area::Location loc)
{
	AliasIter it = std::find_if (_aliases.begin (), _aliases.end (), AreaEq (loc));
	if (it != _aliases.end ())
		it->InitType (type);
	else
		_aliases.push_back (AreaFileData (_uname, type, loc));
}


void FileData::RemoveUnameAlias (Area::Location loc)
{
	AliasIter it = std::find_if (_aliases.begin (), _aliases.end (), AreaEq (loc));
	if (it != _aliases.end ())
	{
		if ( it->GetType ().IsEqual (_type))
			_aliases.erase (it);
		else
			it->SetUniqueName (_uname);
	}

}

void FileData::RemoveTypeAlias (Area::Location loc)
{
	AliasIter it = std::find_if (_aliases.begin (), _aliases.end (), AreaEq (loc));
	if (it != _aliases.end ())
	{
		if (_uname.IsEqual (it->GetUniqueName ()))
			_aliases.erase (it);
		else 
			it->InitType (_type);
	}
}

void FileData::RemoveAlias (Area::Location loc)
{
	AliasIter it = std::find_if (_aliases.begin (), _aliases.end (), AreaEq (loc));
	if (it != _aliases.end ())
		_aliases.erase (it);
}

void FileData::DeepCopy (FileData const & fd)
{
    _state = fd._state;
    _gid = fd._gid;
    _type = fd._type;
    _uname.Init (fd._uname);
	_aliases = fd._aliases;
    _checkSum = fd._checkSum;
}

void FileData::Serialize (Serializer& out) const
{
    _state.Serialize (out);
    out.PutLong (_gid);
    out.PutLong (_type.GetValue ());
    _uname.Serialize (out);
    out.PutLong (_checkSum.GetSum ());
	_aliases.Serialize (out);
	out.PutLong (_checkSum.GetCrc ());
}

void FileData::Deserialize (Deserializer& in, int version)
{
    _state.Deserialize (in, version);
    _gid = in.GetLong ();
	_type.Init (in.GetLong ());
	_uname.Deserialize (in, version);
    unsigned long checkSum = in.GetLong ();
	unsigned long crc = CheckSum::crcWildCard;
	_aliases.Deserialize (in, version);
	if (version > 34)
		crc = in.GetLong ();
	_checkSum.Init (checkSum, crc);
	if (version < 31)
	{
		for (AliasIter it = _aliases.begin (); it != _aliases.end (); ++it)
			it->InitType (_type);
	}
}

void FileData::Verify () const throw (Win::Exception)
{
	std::ostringstream out;
	if (!VerifyFileType ())
	{
		out << *this;
		throw Win::Exception ("Corrupted file data: invalid file type.", out.str ().c_str ());
	}
	if (_gid == 0 && !_type.IsRoot ())
	{
		out << *this;
		throw Win::Exception ("Corrupted file data: invalid global id.", out.str ().c_str ());
	}
	if (_uname.GetName ().empty () && !_type.IsRoot ())
	{
		out << *this;
		throw Win::Exception ("Corrupted file data: invalid file name.", out.str ().c_str ());
	}
}

void FileData::DumpAliases () const
{
	dbg << "Aliases:" << std::endl;
	for (Area::Seq seq; !seq.AtEnd (); seq.Advance ())
	{
		if (IsRenamedIn (seq.GetArea ()))
		{
			UniqueName const & uname = GetUnameIn (seq.GetArea ());
			dbg << "   - in " << Area::Name [seq.GetArea ()] << ": ";
			GlobalIdPack parentGid (uname.GetParentId ());
			dbg << parentGid.ToBracketedString () << "\\" << uname.GetName ();
			dbg << std::endl;
		}
	}
}

std::ostream & operator<<(std::ostream & os, FileData const & fd)
{
	GlobalIdPack fileGid (fd.GetGlobalId ());
	UniqueName const & uname = fd.GetUniqueName ();
	GlobalIdPack parentGid (uname.GetParentId ());
	os << uname.GetName ().c_str () << " - " << fileGid.ToSquaredString ();
	os << "; parent: " << parentGid.ToString () << std::endl;
	CheckSum checksum = fd.GetCheckSum ();
	os << "   Checksum: 0x" << std::hex << checksum.GetSum ();
	os << "; crc: 0x" << std::hex << checksum.GetCrc ();
	os << "; type:" << fd.GetType ().GetName () << std::endl;
	os << "   State: " << fd.GetState () << std::endl;
	if (fd.HasAliases ())
	{
		os << "   Aliases:" << std::endl;
		for (Area::Seq seq; !seq.AtEnd (); seq.Advance ())
		{
			if (fd.IsRenamedIn (seq.GetArea ()))
			{
				UniqueName const & uname = fd.GetUnameIn (seq.GetArea ());
				os << "     - in " << Area::Name [seq.GetArea ()] << ": ";
				GlobalIdPack parentGid (uname.GetParentId ());
				os << parentGid.ToSquaredString () << "\\" << uname.GetName ();
				os << std::endl;
			}
		}
	}
	os << std::endl;
	return os;
}
