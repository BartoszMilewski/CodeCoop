//------------------------------------
//  (c) Reliable Software, 1999 - 2008
//------------------------------------

#include "precompiled.h"
#include "ExportedHistoryTrailer.h"
#include "HistoryNode.h"

#include <Dbg/Assert.h>

void ExportedHistoryTrailer::DirEntry::Serialize (Serializer & out) const
{
	out.PutLong (_gid);
	_offset.Serialize (out);
}

void ExportedHistoryTrailer::DirEntry::Deserialize (Deserializer & in, int version)
{
	_gid = in.GetLong ();
	_offset.Deserialize (in, version);
}

void ExportedHistoryTrailer::ScriptDirEntry::Serialize (Serializer & out) const
{
	DirEntry::Serialize (out);
	out.PutLong (_scriptFlags);
}

void ExportedHistoryTrailer::ScriptDirEntry::Deserialize (Deserializer & in, int version)
{
	DirEntry::Deserialize (in, version);
	_scriptFlags = in.GetLong ();
}

void ExportedHistoryTrailer::RememberScript (GlobalId gid, unsigned long scriptFlags, File::Offset offset)
{
	ScriptDirEntry newEntry (gid, scriptFlags, offset);
	_scriptDir.push_back (newEntry);
}

void ExportedHistoryTrailer::RememberUser (UserId gid, File::Offset offset)
{
	DirEntry newEntry (gid, offset);
	_userDir.push_back (newEntry);
}

class IsEqualId : public std::unary_function<ExportedHistoryTrailer::DirEntry, bool>
{
public:
	explicit IsEqualId (GlobalId id) : _id (id) {}
	bool operator() (ExportedHistoryTrailer::DirEntry const & entry) const
	{
		return entry.GetGid () == _id;
	}
private:
	GlobalId	_id;
};

bool ExportedHistoryTrailer::IsScriptPresent (GlobalId scriptGid, bool & isRejected) const
{
	ScriptIter iter = std::find_if (_scriptDir.begin (), _scriptDir.end (), IsEqualId (scriptGid));
	if (iter != _scriptDir.end ())
	{
		ScriptDirEntry const * script = &(*iter);
		History::Node::State state (script->GetScriptFlags ());
		isRejected = state.IsRejected ();
		return true;
	}
	return false;
}

class IsLessUserId : public std::binary_function<ExportedHistoryTrailer::DirEntry,
												 ExportedHistoryTrailer::DirEntry,
												 bool>
{
public:
	bool operator() (ExportedHistoryTrailer::DirEntry const & item1,
					 ExportedHistoryTrailer::DirEntry const & item2) const
	{
		return item1.GetGid () < item2.GetGid ();
	}
};

UserId ExportedHistoryTrailer::GetHighestUserId () const
{
	typedef std::vector<DirEntry>::const_iterator UserIter;
	UserIter maxUserId = std::max_element (_userDir.begin (), _userDir.end (), IsLessUserId ());
	return maxUserId->GetGid ();
}

//	Returns size of overlap beyond firstCommonScriptId that ends with a lineage script.
//	If the only lineage overlap is firstCommonScriptId, this returns 0
int ExportedHistoryTrailer::GetOverlapLength (GlobalId firstCommonScriptId) const
{
	ScriptIter firstCommon = std::find_if (_scriptDir.begin (), _scriptDir.end (), IsEqualId (firstCommonScriptId));
	Assert (firstCommon != _scriptDir.end ());
	return firstCommon - _scriptDir.begin ();
}

void ExportedHistoryTrailer::PrepareForImport (GlobalId importStartScriptId)
{
	// Remember where in the imported history first common script is located.
	// _scriptDir contains scripts in the reversed chronological order.
	// _scriptDir [0] --> most recent script in the exporter's history
	// _scriptDir [_scriptDir.size () - 1] --> full synch in the exporter's history
	for (unsigned int i = 0; i < _scriptDir.size (); ++i)
	{
		if (_scriptDir [i].GetGid () == importStartScriptId)
		{
			_importStart = i;
			break;
		}
	}
	Assert (_importStart >= 0);
}

File::Offset ExportedHistoryTrailer::GetUserOffset (UserId userId) const
{
	DirIter iter = std::find_if (_userDir.begin (), _userDir.end (), IsEqualId (userId));
	return iter != _userDir.end () ? iter->GetOffset () : File::Offset::Invalid;
}

void ExportedHistoryTrailer::Serialize (Serializer & out) const
{
	out.PutLong (_exporter);
	_projectName.Serialize (out);
	out.PutLong (_scriptDir.size ());
	for (ScriptIter script = _scriptDir.begin (); script != _scriptDir.end (); ++script)
	{
		script->Serialize (out);
	}
	out.PutLong (_userDir.size ());
	for (DirIter user = _userDir.begin (); user != _userDir.end (); ++user)
	{
		user->Serialize (out);
	}
}

void ExportedHistoryTrailer::Deserialize (Deserializer & in, int version)
{
	_importVersion = version;
	_exporter = in.GetLong ();
	_projectName.Deserialize (in, version);
	int dirSize = in.GetLong ();
	ScriptDirEntry scriptTmpEntry (gidInvalid, 0, File::Offset::Invalid);
	int i = 0;
	for (i = 0; i < dirSize; ++i)
	{
		scriptTmpEntry.Deserialize (in, version);
		_scriptDir.push_back (scriptTmpEntry);
	}
	DirEntry tmpEntry (gidInvalid, File::Offset::Invalid);
	dirSize = in.GetLong ();
	for (i = 0; i < dirSize; ++i)
	{
		tmpEntry.Deserialize (in, version);
		_userDir.push_back (tmpEntry);
	}
}
