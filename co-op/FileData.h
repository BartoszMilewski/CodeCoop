#if !defined (FILEDATA_H)
#define FILEDATA_H
//----------------------------------
// (c) Reliable Software 1997 - 2005
//----------------------------------

#include "UniqueName.h"
#include "SerString.h"
#include "GlobalId.h"
#include "Serialize.h"
#include "FileState.h"
#include "FileTypes.h"
#include "Global.h"
#include "Area.h"
#include "SerList.h"
#include "CheckSum.h"

#include <iosfwd>

class FileChanges;

class AreaFileData
{
public:
	AreaFileData (Deserializer& in, int version)
	{
		Deserialize (in, version);
	}
	AreaFileData (UniqueName const & uname, FileType type, Area::Location loc)
		: _uname (uname), _type (type), _location (loc)
	{}
	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);
	Area::Location GetLocation () const { return _location; }
	UniqueName const & GetUniqueName () const { return _uname; }
	void SetUniqueName (UniqueName const & uname) {_uname.Init (uname);}
	FileType const & GetType () const { return _type ;}
	void InitType (FileType type) { _type = type;}

private:
	Area::Location	_location;
	UniqueName		_uname;
	FileType        _type;
};

class FileData : public Serializable
{
public:
	// Copy constructor
	FileData (FileData const & fd)
		: _state (fd._state),
		  _gid (fd._gid),
		  _type (fd._type),
		  _uname (fd._uname),
		  _aliases (fd._aliases),
		  _checkSum (fd._checkSum)
	{}
	// Default constructor
	FileData ()
		: _gid (gidInvalid),
		  _state (),
		  _type ()
	{}

	FileData (Deserializer & in, int version)
	{
		Deserialize (in, version);
	}

	FileData (UniqueName const & uname, GlobalId gidFile, FileState state, FileType type)
		: _state (state), 
		  _gid (gidFile), 
		  _type (type),
		  _uname (uname)
	{}
	void DeepCopy (FileData const & fd);
	void operator=(FileData const & fd)		// Required when FileData is placed in STL containers
	{
		DeepCopy (fd);
	}

	std::string const & GetName () const { return _uname.GetName (); }
	UniqueName const & GetUniqueName () const { return _uname; }
	void SetName (GlobalId gidParent, std::string const & name) { _uname.Init (gidParent, name); }
	void SetName (UniqueName const & uname) { _uname.Init (uname); }
	void Rename (UniqueName const & uname);
	void ChangeType (FileType const & type);
	GlobalId GetGlobalId () const { return _gid; }
	FileState GetState () const { return _state; }
	FileType GetType () const { return _type; }
	FileType & GetTypeEdit () { return _type; }
	bool HasAliases () const { return !_aliases.empty (); }
	bool IsRenamedIn (Area::Location loc) const;
	bool IsTypeChangeIn (Area::Location loc) const;
	bool IsRenamedAs (UniqueName const & uname) const;
	UniqueName const & GetUnameIn (Area::Location loc) const;
	FileType const & GetTypeIn (Area::Location loc) const;
	CheckSum GetCheckSum () const	{ return _checkSum; }
	bool IsCheckedIn () const { return _state.IsCheckedIn (); }
	bool IsParent (GlobalId parentId) const { return _uname.GetParentId () == parentId; }
	void GetNonContentsFileChanges (FileChanges & file) const;

	void SetGid (GlobalId gid) { _gid = gid; }
	void SetState (FileState state) { _state = state; }
	void SetType (FileType type) { _type = type; }
	void SetCheckSum (CheckSum checkSum) { _checkSum = checkSum; }
	// Alias manipulation
	void AddUnameAlias (UniqueName const & uname, Area::Location loc);
	void FileData::AddTypeAlias (FileType const & type, Area::Location loc);
	void RemoveUnameAlias (Area::Location loc);
	void RemoveTypeAlias (Area::Location loc);
	void RemoveAlias (Area::Location loc);
	void ClearAliases () { _aliases.clear (); }

	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);
	bool VerifyFileType () const { return _type.Verify (); }
	void Verify () const throw (Win::Exception);
	void DumpAliases () const;

protected:
	typedef std::list<AreaFileData>::iterator AliasIter;
	typedef std::list<AreaFileData>::const_iterator AliasIterC;

protected:
	GlobalId	_gid;
	UniqueName	_uname;
	FileState	_state;
	FileType	_type;
	SerList<AreaFileData> _aliases;
	CheckSum	_checkSum;
};

std::ostream & operator<<(std::ostream & os, FileData const & fd);

#endif
