#if !defined (EXPORTEDHISTORYTRAILER_H)
#define EXPORTEDHISTORYTRAILER_H
//------------------------------------
//  (c) Reliable Software, 1999 - 2008
//------------------------------------

#include "Serialize.h"
#include "Params.h"
#include "GlobalId.h"
#include "SerString.h"
#include "XFileOffset.h"

class ExportedHistoryTrailer : public Serializable
{
	friend class IsEqualId;
	friend class ImportedHistorySeq;

public:
	ExportedHistoryTrailer (UserId exporter, std::string const & projectName)
		: _exporter (exporter),
		  _projectName (projectName),
		  _importStart (0),
		  _importVersion (modelVersion)
	{}
	ExportedHistoryTrailer (Deserializer & in)
		: _importStart (0),
		  _importVersion (modelVersion)
	{
		Read (in);
	}

	void RememberScript (GlobalId gid, unsigned long scriptFlags, File::Offset offset);
	void RememberUser (UserId gid, File::Offset offset);

	bool IsScriptPresent (GlobalId scriptGid, bool & isRejected) const;
	bool IsFromVersion40 () const { return _importVersion < 37; }

	int GetImportVersion () const { return _importVersion; }
	int GetScriptCount () const { return _scriptDir.size (); }
	int GetUserCount () const { return _userDir.size (); }
	std::string const & GetProjectName () const { return _projectName; }
	UserId GetExporterId () const { return _exporter; }
	UserId GetHighestUserId () const;
	int GetOverlapLength (GlobalId firstCommonScriptId) const;
	void PrepareForImport (GlobalId importStartScriptId);
	File::Offset GetUserOffset (UserId userId) const;

    // Serializable interface

    int  VersionNo () const { return modelVersion; }
    int  SectionId () const { return 'XHDR'; }
    bool IsSection () const { return true; }
    void Serialize (Serializer & out) const;
    void Deserialize (Deserializer & in, int version);

public:
	class DirEntry : public Serializable
	{
	public:
		DirEntry (GlobalId gid, File::Offset offset)
			: _gid (gid),
			  _offset (offset)
		{}
		GlobalId GetGid () const { return _gid; }
		File::Offset GetOffset () const { return _offset; }

		void Serialize (Serializer & out) const;
		void Deserialize (Deserializer & in, int version);

	private:
		GlobalId		_gid;
		SerFileOffset	_offset;
	};

	typedef std::vector<DirEntry>::const_iterator DirIter;

private:
	class ScriptDirEntry : public DirEntry
	{
	public:
		ScriptDirEntry (GlobalId gid, unsigned long scriptFlags, File::Offset offset)
			: DirEntry (gid, offset),
			  _scriptFlags (scriptFlags)
		{}
		unsigned long GetScriptFlags () const { return _scriptFlags; }

		void Serialize (Serializer & out) const;
		void Deserialize (Deserializer & in, int version);

	private:
		unsigned long	_scriptFlags;
	};

	typedef std::vector<ScriptDirEntry>::const_iterator ScriptIter;

private:
    // persistent
	UserId						_exporter;
	SerString					_projectName;
	std::vector<ScriptDirEntry>	_scriptDir;
	std::vector<DirEntry>		_userDir;
    // volatile
	int							_importStart;
	int							_importVersion;
};

class ImportedHistorySeq
{
public:
    ImportedHistorySeq (ExportedHistoryTrailer const & trailer)
		: _cur (trailer._scriptDir.size () - 1),
		  _end (trailer._importStart),
		  _scriptDir (trailer._scriptDir)
	{}

	void Advance () { --_cur; }
    bool AtEnd () const { return _cur < _end || _cur == 0xffffffff; }

	GlobalId GetScriptId () const { return _scriptDir [_cur].GetGid (); }
	File::Offset GetScriptOffset () const { return _scriptDir [_cur].GetOffset (); }
	unsigned long GetScriptFlags () const { return _scriptDir [_cur].GetScriptFlags (); }
	bool IsHistoryBegin () const { return _cur == _scriptDir.size () - 1; }

private:
	unsigned int	_cur;
	unsigned int	_end;
	std::vector<ExportedHistoryTrailer::ScriptDirEntry>	const & _scriptDir;
};

#endif
