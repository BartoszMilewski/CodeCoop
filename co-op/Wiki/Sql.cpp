//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

#include "precompiled.h"

#include "Sql.h"
#include <File/Dir.h>
#include <File/FileIo.h>

namespace Sql
{
	class RecordParser
	{
	private:
		class Line
		{
		public:
			Line (std::string const & line);
			std::string PropName () const;
			std::string PropValue () const;
			bool IsClosed () const { return _isClosed; }
		private:
			std::string const & _line;
			unsigned	_bar1;
			unsigned	_bar2;
			unsigned	_bar3;
			bool		_isClosed;
		};
	public:
		RecordParser (std::istream & in)
			: _in (in)
		{}
		// fields == 0 => read all fiels, preamble == 0 => skip preamble
		void Parse (TuplesVector & tuples, 
					NocaseSet const * fields,
					std::string * preamble = 0,
					std::string * trailer = 0
					);
	private:
		std::istream & _in;
	};
}

Sql::RecordParser::Line::Line (std::string const & line)
	: _line (line), _bar1 (0), _bar2 (0), _bar3 (0), _isClosed (false)
{
	Assert (line.size () != 0);
	Assert (line [_bar1] == '|');
	if (line.size () <= 1)
		return;
	unsigned i = _line.find ('|', _bar1 + 1);
	if (i != std::string::npos)
	{
		_bar2 = i;
		_bar3 = i;
		if (_line.size () == _bar2 + 1)
			return;
		i = _line.find ('|', _bar2 + 1);
		if (i == std::string::npos)
			_bar3 = line.size (); // one beyond the end
		else
		{
			_bar3 = i;
			_isClosed = true;
		}
	}
}

std::string Sql::RecordParser::Line::PropName () const
{
	unsigned i = _bar1 + 1;
	// skip leading white
	while (i < _bar2 && IsSpace (_line [i]))
		++i;
	if (i >= _bar2) // empty
		return std::string ();
	unsigned j = _bar2 - 1;
	while (IsSpace (_line [j]))
		--j;
	return _line.substr (i, j - i + 1);
}

std::string Sql::RecordParser::Line::PropValue () const
{
	if (_bar2 == _bar3)
		return std::string ();
	return _line.substr (_bar2 + 1, _bar3 - _bar2 - 1);
}

// If preamble or trailer non-null, they will be filled with pre-table and post-table contents.
// If fields is non-null, only specified fields will be copied into tuples.
// If fields in null, all fields will be copied.

void Sql::RecordParser::Parse (TuplesVector & tuples, 
							   NocaseSet const * fields,
							   std::string * preamble,
							   std::string * trailer)
{
	std::string line;
	if (!getline (_in, line))
		return;
	// skip non-fields
	while (line.size () == 0 || line [0] != '|')
	{
		if (preamble)
		{
			*preamble += line;
			*preamble += "\n";
		}
		if (!getline (_in, line))
			return;
	}

	while (line.size () != 0 && line [0] == '|')
	{
		RecordParser::Line lineParser (line);
		std::string name = lineParser.PropName ();
		std::string value = lineParser.PropValue ();
		bool isClosed = lineParser.IsClosed ();
		bool isRelevant = false;
		if (!name.empty () && (fields == 0 || fields->find (name) != fields->end ()))
		{
			tuples.push_back (std::make_pair (name, value));
			isRelevant = true;
		}
		if (!getline (_in, line))
			return;
		if (!isClosed)
		{
			// multi-line value
			do
			{
				isClosed = (line.size () > 0 && line [line.size () - 1] == '|'); // trailing '|'
				if (isRelevant)
				{
					std::string & editValue = tuples.back ().second;
					editValue += "\n";
					editValue += line;
					if (isClosed)
					{
						editValue.resize (editValue.size () - 1);
					}
				}
				if (!getline (_in, line))
					return;
			} while (!isClosed);
		}
	}
	if (trailer)
	{
		do
		{
			*trailer += line;
			*trailer += "\n";
		} while (getline (_in, line));
	}
}

Sql::Listing::RecordFile::RecordFile (std::string const & fullPath)
{
	PathSplitter splitter (fullPath);
	_fileName.assign (splitter.GetFileName ());
	_fileName += splitter.GetExtension ();
	if (File::Exists (fullPath))
	{
		FileInfo fileInfo (fullPath);
		_time = FileTime (fileInfo);
	}
	else
		_time = FileTime ();
}

//  non-empty field is less than empty
bool Sql::Record::IsLess (Record const & rec, std::string const & fieldName) const
{
	TuplesMap::const_iterator it1 = _tuples.find (fieldName);
	TuplesMap::const_iterator it2 = rec._tuples.find (fieldName);
	if (it2 == rec._tuples.end ())
		return it1 != _tuples.end ();
	if (it1 == _tuples.end ())
		return false;
	return IsNocaseLess (it1->second, it2->second);
}

Sql::ExistingRecord::ExistingRecord (std::string const & fullPath, Sql::Listing::RecordFile const & recordFile)
: Record (&recordFile)
{
	InStream in (fullPath);
	RecordParser parser (in);
	TuplesVector tupleVec;
	parser.Parse (tupleVec, 0);
	_tuples [FileNamePseudoProp] = _file->GetFileName ();
	for (TuplesVector::const_iterator tit = tupleVec.begin (); tit != tupleVec.end (); ++tit)
	{
		TrimmedString value (tit->second);
		_tuples [tit->first] = value;
	}
}

Sql::WritableRecord::WritableRecord (std::string const & path)
: _path (path)
{
	InStream in (_path);
	Sql::RecordParser parser (in);
	parser.Parse (_tuples, 0, &_preamble, &_trailer);
}

void Sql::WritableRecord::Update (NocaseMap<std::string> & inTuples)
{
	for (TuplesVector::iterator it = _tuples.begin (); it != _tuples.end (); ++it)
	{
		NocaseMap<std::string>::iterator hit = inTuples.find (it->first);
		if (hit != inTuples.end ())
		{
			it->second = hit->second;
			inTuples.erase (hit);
		}
	}
	std::copy (inTuples.begin (), inTuples.end (), std::back_inserter (_tuples));
}

void Sql::WritableRecord::Save ()
{
	OutStream out (_path);
	out << _preamble;
	for (TuplesVector::const_iterator it = _tuples.begin (); it != _tuples.end (); ++ it)
	{
		out << "| " << it->first << " |" << it->second << "|" << std::endl;
	}
	out << _trailer;
}

Sql::Listing::Listing (FilePath const & curPath, 
					   FilePath const & sysPath, 
					   std::string const & tableName,
					   bool isSystemTable)
	: _path (isSystemTable? sysPath: curPath), _tableName (tableName)
{
	_path.DirDown (tableName.c_str ());

	if (!File::Exists (_path.GetDir ()))
		return;
	for (FileSeq dir (_path.GetFilePath ("*.wiki")); !dir.AtEnd (); dir.Advance ())
	{
		if (IsNocaseEqual (dir.GetName (), "template.wiki"))
			continue;
		if (IsNocaseEqual (dir.GetName (), "index.wiki"))
			continue;
		std::string recordFileName (dir.GetName ());
		Assert (recordFileName.size () > 5);
		RecordFile file (recordFileName, dir.GetWriteTime ());
		_files.push_back (file);
	}
}

Sql::Table::Table (FilePath const & curPath,
				   FilePath const & sysPath,
				   Sql::SelectCommand const & cmd)
: Listing (curPath, sysPath, cmd.GetTableName (), cmd.IsSystemTable ())
{
	DefaultSort ();
	NocaseSet fields;
	std::vector<std::string> const & selection = cmd.GetSelectFields ();
	std::copy (selection.begin (), selection.end (), std::inserter (fields, fields.begin ()));
	if (cmd.HasSort ())
		fields.insert (cmd.GetSortField ());
	Node const * predicate = cmd.GetPredicate ();
	if (predicate != 0)
		predicate->GatherFields (fields);
	for (RevFileIterator it = rbeginFile (); it != rendFile (); ++it)
	{
		std::string name (it->GetFileName ());
		std::unique_ptr<Sql::Record> record (new Record (&*it));
		InStream in (_path.GetFilePath (name));
		RecordParser parser (in);
		TuplesVector tuples;
		parser.Parse (tuples, &fields);
		TuplesMap & tuplesMap = record->GetTuples ();
		if (fields.find (FileNamePseudoProp) != fields.end ())
			tuplesMap [FileNamePseudoProp] = name;
		for (TuplesVector::const_iterator tit = tuples.begin (); tit != tuples.end (); ++tit)
		{
			TrimmedString value (tit->second);
			tuplesMap [tit->first] = value;
		}

		if (predicate == 0 || predicate->Match (record->GetTuples ()))
			_records.push_back (std::move(record));
	}
	if (cmd.HasSort ())
		Sort (cmd.GetSortField (), cmd.IsSortDescending ());
}

class IsRecordLess
{
public:
	IsRecordLess (std::string const & fieldName)
		: _field (fieldName)
	{}
	bool operator () (Sql::Record const * rec1, Sql::Record const * rec2) const
	{
		return rec1->IsLess (*rec2, _field);
	}
private:
	std::string _field;
};

void Sql::Table::Sort (std::string const & fieldName, bool isDescending)
{
	std::stable_sort (_records.begin (), _records.end (), IsRecordLess (fieldName));
	if (isDescending)
		std::reverse (_records.begin (), _records.end ());
}

unsigned Sql::Listing::NextRecordId () const
{
	long maxNum = 0;
	for (std::vector<RecordFile>::const_iterator it = _files.begin (); it != _files.end (); ++it)
	{
		std::string numeral = it->GetRecordName ();
		char * endPtr;
		long n = std::strtol (numeral.c_str (), &endPtr, 10);
		if (endPtr - numeral.c_str () == numeral.size () && n > maxNum)
			maxNum = n;
	}
	return maxNum + 1;
}

inline bool LaterFileTime (Sql::Table::RecordFile const & f1, Sql::Table::RecordFile const & f2)
{
	return f1.GetFileTime () < f2.GetFileTime ();
}

void Sql::Listing::DefaultSort ()
{
	std::sort (_files.begin (), _files.end (), &LaterFileTime);
}
