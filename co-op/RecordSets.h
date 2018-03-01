#if !defined (RECORDSETS_H)
#define RECORDSETS_H
//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "RecordSet.h"
#include "FileState.h"
#include "FileTypes.h"
#include "MailboxScriptState.h"
#include "HistoryScriptState.h"
#include "ProjectState.h"
#include "MergeStatus.h"

#include <auto_vector.h>

#include <File/File.h>

#include <iosfwd>

class GidToStringCache
{
public:
	GidToStringCache ()
		: _unknownString ("Unknown")
	{}

	void AddString (GlobalId gid, std::string const & string);
	std::string const & GetString (GlobalId gid) const;
	void Clear () { _strings.clear (); }

private:
	std::map<GlobalId, std::string>	_strings;
	std::string						_unknownString;
};

class FileRecordSet: public RecordSet
{
public:
	FileRecordSet (Table const & table)
		: RecordSet (table)
	{}
	GlobalId GetParentId (unsigned int row) const;
	unsigned long GetState (unsigned int row) const { return _fileState [row].GetValue (); }
	unsigned long GetType (unsigned int row) const { return _types [row].GetValue (); }

protected:
	char const * GetFileName (GlobalId gid) const;
	char const * GetParentPath (unsigned int row) const;
	void UpdatePaths ();
	int CmpNames (unsigned int row1, unsigned int row2) const;
	int CmpStates (unsigned int row1, unsigned int row2) const;
	int CmpGids (unsigned int row1, unsigned int row2) const;
	int CmpTypes (unsigned int row1, unsigned int row2) const;
	int CmpPaths (unsigned int row1, unsigned int row2) const;
	int CmpChanged (unsigned int row1, unsigned int row2) const;

	bool AreOfSameType (unsigned int row1, unsigned int row2) const;
	int CmpMixedType (unsigned int row1, unsigned int row2) const;

protected:
	GidList 				_parentIds;
	std::vector<FileType>	_types;
	std::vector<FileState>	_fileState;
	GidToStringCache		_pathCache;
};

class ScriptRecordSet: public RecordSet
{
public:
	ScriptRecordSet (Table const & table)
		: RecordSet (table)
	{}

	unsigned long GetState (unsigned int row) const { return _scriptState [row]; }
	char const * GetSenderName (unsigned int row) const;

protected:
	void UpdateSenders ();
	void MultiLine2SingleLine (std::string & buf) const;

protected:
	std::vector<unsigned long>	_scriptState;
	std::vector<std::string>	_timeStamps;
	GidToStringCache			_senderCache;
};

class MailBoxRecordSet: public ScriptRecordSet
{
public:
	MailBoxRecordSet (Table const & table, Restriction const & restrict);
	void CopyStringField (unsigned int row, unsigned int col, unsigned int bufLen, char * buf) const;
	void GetImage (unsigned int row, SelectState state, int & iImage, int & iOverlay) const;
	void Refresh (unsigned int row);
	bool IsEmpty () const;
	bool IsDefaultSelection () const;
	unsigned GetDefaultSelectionRow () const;
	bool SupportsDrawStyles () const { return true; }
	DrawStyle GetStyle (unsigned int row) const;
	void DumpField (std::ostream & out, unsigned int row, unsigned int col) const;

protected:
	void PushRow (GlobalId gid, std::string const & name);
	void PopRows (int oldSize);
};

class FolderRecordSet: public FileRecordSet
{
public:
	FolderRecordSet (Table const & table, Restriction const & restrict);
	DegreeOfInterest HowInteresting () const;
	void CopyStringField (unsigned int row, unsigned int col, unsigned int bufLen, char * buf) const;
	std::string GetStringField (unsigned int row, unsigned int col) const;
	void GetImage (unsigned int row, SelectState state, int & iImage, int & iOverlay) const;
	void Refresh (unsigned int row);
	int CmpRows (unsigned int row1, unsigned int row2, unsigned int col) const;
	void BeginNewItemEdit ();
	void AbortNewItemEdit ();
	void DumpField (std::ostream & out, unsigned int row, unsigned int col) const;
	bool IsEqual (RecordSet const * recordSet) const;
	void Verify () const;
	//
	// TransactionObserver interface
	//
	void CommitUpdate (bool delayRefresh);

protected:
	void PushRow (GlobalId gid, std::string const & name);
	void PopRows (int oldSize);

private:
	std::vector<bool>			_isReadOnly;
	std::vector<FileTime>		_dateModified;
	std::vector<unsigned long>	_size;
};

class CheckInRecordSet: public FileRecordSet
{
public:
	CheckInRecordSet (Table const & table, Restriction const & restrict);
	DegreeOfInterest HowInteresting () const;
	void CopyStringField (unsigned int row, unsigned int col, unsigned int bufLen, char * buf) const;
	void GetImage (unsigned int row, SelectState state, int & iImage, int & iOverlay) const;
	void Refresh (unsigned int row);
	int CmpRows (unsigned int row1, unsigned int row2, unsigned int col) const;
	void DumpField (std::ostream & out, unsigned int row, unsigned int col) const;

protected:
	void PushRow (GlobalId gid, std::string const & name);
	void PopRows (int oldSize);
private:
	std::vector<FileTime>		_dateModified;

};

class SynchRecordSet: public FileRecordSet
{
public:
	SynchRecordSet (Table const & table, Restriction const & restrict);
	void CopyStringField (unsigned int row, unsigned int col, unsigned int bufLen, char * buf) const;
	void GetImage (unsigned int row, SelectState state, int & iImage, int & iOverlay) const;
	bool SupportsDrawStyles () const { return true; }
	DrawStyle GetStyle (unsigned int row) const;
	void Refresh (unsigned int row);
	int CmpRows (unsigned int row1, unsigned int row2, unsigned int col) const;
	void DumpField (std::ostream & out, unsigned int row, unsigned int col) const;

protected:
	void PushRow (GlobalId gid, std::string const & name);
	void PopRows (int oldSize);
};

class HistoryRecordSet: public ScriptRecordSet
{
public:
	HistoryRecordSet (Table const & table, Restriction const & restrict);
	char const * GetSenderName (unsigned int row) const;
	GlobalId GetPredecessorId (unsigned int row) const;
	DegreeOfInterest HowInteresting () const { return NotInteresting; }
	void CopyStringField (unsigned int row, unsigned int col, unsigned int bufLen, char * buf) const;
	void GetImage (unsigned int row, SelectState state, int & iImage, int & iOverlay) const;
	bool SupportsDrawStyles () const { return true; }
	DrawStyle GetStyle (unsigned int row) const;
	void DumpField (std::ostream & out, unsigned int row, unsigned int col) const;
};

class ScriptDetailsRecordSet : public FileRecordSet
{
public:
	ScriptDetailsRecordSet (Table const & table, Restriction const & restrict);
	DegreeOfInterest HowInteresting () const { return NotInteresting; }
	void CopyStringField (unsigned int row, unsigned int col, unsigned int bufLen, char * buf) const;
	void GetImage (unsigned int row, SelectState state, int & iImage, int & iOverlay) const;
	bool IsEqual (RecordSet const * recordSet) const;
	void Refresh (unsigned int row);
	int CmpRows (unsigned int row1, unsigned int row2, unsigned int col) const;
	bool SupportsDrawStyles () const { return true; }
	DrawStyle GetStyle (unsigned int row) const;
	void DumpField (std::ostream & out, unsigned int row, unsigned int col) const;

	unsigned long GetState (unsigned int row) const { return _mergeStatus [row].GetValue (); }

private:
	std::vector<MergeStatus>	_mergeStatus;
};

class MergeDetailsRecordSet : public FileRecordSet
{
public:
	MergeDetailsRecordSet (Table const & table, Restriction const & restrict);
	DegreeOfInterest HowInteresting () const { return NotInteresting; }
	void CopyStringField (unsigned int row, unsigned int col, unsigned int bufLen, char * buf) const;
	void GetImage (unsigned int row, SelectState state, int & iImage, int & iOverlay) const;
	bool IsEqual (RecordSet const * recordSet) const;
	void Refresh (unsigned int row);
	int CmpRows (unsigned int row1, unsigned int row2, unsigned int col) const;
	bool SupportsDrawStyles () const { return true; }
	DrawStyle GetStyle (unsigned int row) const;
	void DumpField (std::ostream & out, unsigned int row, unsigned int col) const;

	unsigned long GetState (unsigned int row) const { return _mergeStatus [row].GetValue (); }

private:
	std::vector<MergeStatus>	_mergeStatus;
	std::vector<std::string>	_targetPaths;
};

class ProjectRecordSet : public RecordSet
{
public:
	ProjectRecordSet (Table const & table, Restriction const & restrict);
	DegreeOfInterest HowInteresting () const;
	unsigned long GetState (unsigned int row) const { return _projState [row].GetValue (); }
	void CopyStringField (unsigned int row, unsigned int col, unsigned int bufLen, char * buf) const;
	void GetImage (unsigned int row, SelectState state, int & iImage, int & iOverlay) const;
	void Refresh (unsigned int row);
	bool IsEqual (RecordSet const * recordSet) const;
	int CmpRows (unsigned int row1, unsigned int row2, unsigned int col) const;
	void DumpField (std::ostream & out, unsigned int row, unsigned int col) const;

protected:
	void PushRow (GlobalId gid, std::string const & name);
	void PopRows (int oldSize);

private:
	std::vector<Project::State> _projState;
	std::vector<FileTime>		_lastModified;
};

class WikiRecordSet: public RecordSet
{
public:
	WikiRecordSet (Table const & table);
	std::string GetStringField (unsigned int row, unsigned int col) const;
private:
	std::vector<std::string> _urls;
};

class EmptyRecordSet : public RecordSet
{
public:
	EmptyRecordSet (Table const & table);

	bool IsEmpty () const { return true; }
	void CopyStringField (unsigned int row, unsigned int col, unsigned int bufLen, char * buf) const;

protected:
	void PushRow (GlobalId gid, std::string const & name) {}
	void PopRows (int oldSize) {}
};

#endif
