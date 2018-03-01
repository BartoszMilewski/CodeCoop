#if !defined (SYNCHAREA_H)
#define SYNCHAREA_H
//------------------------------------
//	(c) Reliable Software, 1997 - 2008
//------------------------------------

#include "Transact.h"
#include "Table.h"
#include "XArray.h"
#include "XString.h"
#include "XLong.h"
#include "SynchKind.h"
#include "FileTypes.h"
#include "Params.h"
#include "SerString.h"
#include "CheckSum.h"
#include "Global.h"

class DataBase;
class PathFinder;
class FileData;
class FileCmd;

class SynchScriptInfo : public Serializable
{
public:
	SynchScriptInfo (Deserializer& in, int version)
	{
		Deserialize (in, version);
	}

	GlobalId GetScriptId () const { return _scriptId; }
	std::string const & GetScriptFileName () const { return _name; }

	// Serializable interface

	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);

private:
	GlobalId  _scriptId;
	int 	  _refCount;
	SerString _name;
};

// SynchItem describes a file changed by the incoming script or
// file whose changes have been undone because of script conflict
class SynchItem : public Serializable
{
public:
	SynchItem (FileCmd const & fileCmd);
	SynchItem (FileData const & fileData, CheckSum checkSum);
	SynchItem (Deserializer& in, int version)
	{
		Deserialize (in, version);
	}
	// Copy contructor used by transactable array
	SynchItem (SynchItem const & item)
		: _parentGid (item.GetParentGid ()),
		  _fileGid (item.GetGlobalId ()),
		  _synchKind (item.GetSynchKind ()),
		  _name (item.GetName ()),
		  _type (item.GetType ()),
		  _checkSum (item.GetCheckSum ())
	{}

	GlobalId GetParentGid () const { return _parentGid; }
	std::string const & GetName () const { return _name; }
	GlobalId GetGlobalId () const { return _fileGid; }
	SynchKind GetSynchKind () const { return _synchKind; }
	FileType GetType () const { return _type; }
	CheckSum GetCheckSum () const { return _checkSum; }

	void SetCheckSum (CheckSum newChecksum) { _checkSum = newChecksum; }
	void SetName (std::string const & name) { _name = name; }
	void SetParentId (GlobalId gid) { _parentGid = gid; }
	void SetSynchKind (SynchKind kind) { _synchKind = kind; }

	// Serializable interface

	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);

private:
	GlobalId	_parentGid;
	GlobalId	_fileGid;
	SynchKind	_synchKind;
	SerString	_name;
	FileType	_type;
	CheckSum	_checkSum;
};

class SynchArea : public Table, public TransactableContainer
{
	friend class XSynchAreaSeq;
	friend class SynchAreaSeq;

public:
	SynchArea (DataBase const & dataBase, PathFinder & pathFinder);

	void XAddScriptFile (std::string const & scriptComment, GlobalId scriptId);
	void XAddSynchFile (FileCmd const & fileCmd);
	void XAddSynchFile (FileData const & trans, CheckSum checkSum);

	CheckSum XGetCheckSum (GlobalId gid) const;
	SynchKind XGetSynchKind (GlobalId gid) const;
	void XUpdateCheckSum (GlobalId gid, CheckSum newChecksum);
	void XUpdateName (GlobalId gid, std::string const & newName);
	void XRemoveSynchFile (GlobalId gid);
	int XCount () const { return _synchItems.XCount (); }

	int Count () const { return _synchItems.Count(); }
	bool IsEmpty () const { return _synchItems.Count() == 0; }
	bool IsEmpty (std::string const & curOperation) const;
	bool IsPresent (GlobalId gid) const;
	void Verify (PathFinder & pathFinder) const;
	void GetFileList (GidList & files) const;

	// Table interface

	void QueryUniqueIds (Restriction const & restrict, GidList & ids) const;
	Table::Id GetId () const { return Table::synchTableId; }
	bool IsValid () const;
	std::string GetStringField (Column col, GlobalId gid) const;
	std::string GetStringField (Column col, UniqueName const & uname) const;
	GlobalId	GetIdField (Column col, UniqueName const & uname) const;
	GlobalId	GetIdField (Column col, GlobalId gid) const;
	unsigned long GetNumericField (Column col, GlobalId gid) const;

	std::string GetCaption (Restriction const & restrict) const;

	// Transactable interface

	bool IsSection () const { return true; }
	int  SectionId () const { return 'SYNC'; }
	int  VersionNo () const { return modelVersion; }
	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);

private:
	SynchItem const * XFindSynchItem (GlobalId gid) const;
	SynchItem * XFindEditSynchItem (GlobalId gid);
	SynchItem const * FindSynchItem (GlobalId gid) const;
	void XRemoveSynchItem (GlobalId gid);
	void XSetParentIds ();
	void CheckForDuplicates (SynchItem const & newItem) const;

private:
	static char const *				_kindName [];

	DataBase const				  & _dataBase;
	PathFinder					  & _pathFinder;
	XLong							_scriptId;
	XString							_scriptComment;
	TransactableArray<SynchItem>	_synchItems;
};

class XSynchAreaSeq
{
public:
	XSynchAreaSeq (SynchArea const & synchArea)
		: _cur (0),
		  _synchItems (synchArea._synchItems)
	{}

	bool AtEnd () const 		  { return _cur == _synchItems.XCount (); }
	void Advance () 			  { _cur++; }
	void Rewind ()				  { _cur = 0; }

	GlobalId GetGlobalId () const { return _synchItems.XGet (_cur)->GetGlobalId (); }

private:
	int _cur;
	TransactableArray<SynchItem> const & _synchItems;
};

class SynchAreaSeq
{
public:
	SynchAreaSeq (SynchArea const & synchArea)
		: _cur (0),
		  _synchItems (synchArea._synchItems)
	{}

	bool AtEnd () const 		  { return _cur == _synchItems.Count (); }
	void Advance () 			  { _cur++; }
	void Rewind ()				  { _cur = 0; }

	GlobalId GetGlobalId () const { return _synchItems.Get (_cur)->GetGlobalId (); }

private:
	int _cur;
	TransactableArray<SynchItem> const & _synchItems;
};

#endif
