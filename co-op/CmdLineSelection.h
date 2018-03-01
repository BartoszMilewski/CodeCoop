#if !defined (CMDLINESELECTION_H)
#define CMDLINESELECTION_H
//----------------------------------
// (c) Reliable Software 1998 - 2006
//----------------------------------

#include "SelectionMan.h"
#include "RecordSet.h"
#include "UniqueName.h"
#include "Directory.h"
#include "GlobalId.h"

#include <File/Path.h>

#include <iosfwd>

class TableProvider;
class PathSequencer;

class PathParser
{
public:
	PathParser (Directory const & folder);
	UniqueName const * Convert (char const * fullPath);

private:
	Directory::Sequencer 	_folder;
	FilePath				_root;
	UniqueName				_uname;
};

class CmdLineSelection: public SelectionManager
{
public:
	CmdLineSelection (TableProvider & tableProvider, Directory & folder)
		: _tableProvider (tableProvider),
		  _folder (folder)
	{}

	void SetSelection (PathSequencer & sequencer);
	void SetSelection (GidList const & ids);
	void SelectAll () {}
	RecordSet const * GetRecordSet (Table::Id tableId) const;
	void DumpRecordSet (std::ostream & out, Table::Id tableId, bool allRows);
	void VerifyRecordSet () const {}
	void GetRows (std::vector<unsigned> & rows, Table::Id tableId) const;
	void GetSelectedRows (std::vector<unsigned> & rows) const { return; }
	void GetSelectedRows (std::vector<unsigned> & rows, Table::Id tableId) const;
	void Clear ();
	unsigned int SelCount () const { return _names.size () + _ids.size (); }
	bool IsDefaultSelection () const { return true; }
	bool IsInProject () const { return true; }

private:
	typedef std::set<UniqueName> UnameSet;

	TableProvider &						_tableProvider;
	Directory &							_folder;
	mutable std::unique_ptr<RecordSet>	_recordSet;
	std::vector<UniqueName>				_names;
	UnameSet							_nameSet;
	GidList								_ids;
};

#endif
