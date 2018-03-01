#if !defined (SQL_H)
#define SQL_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------
#include "SqlNode.h"
#include <File/File.h>
#include <File/Path.h>
#include <StringOp.h>
#include <auto_vector.h>

namespace Sql
{
	char const FileNamePseudoProp [] = "**FILENAME**";
	char const DeletePseudoProp [] = "**DELETE**";
	void Canonicalize (std::string & name);
	bool IsPseudo (std::string const & name);
	std::string DeCanonicalize (std::string const & name);

	//-------------
	// Sql Commands
	//-------------
	class Command
	{
	public:
		enum Type { Insert, From, Select, Invalid };
		void AddPredicate (std::unique_ptr<Node> predicate)
		{
			_predicate = std::move(predicate);
		}
		Node const * GetPredicate () const { return _predicate.get (); }
		void Dump (std::ostream & out) const 
		{
			if (_predicate.get () != 0)
			{
				out << "<pre>";
				_predicate->Dump (out, 0); 
				out << "</pre>";
			}
		}
	protected:
		Command (std::string const & tableName, bool isSystemTable)
			: _table (tableName), _isSystemTable (isSystemTable)
		{}
		Command () {}
	public:
		virtual ~Command () {}
		virtual Type GetType () const = 0;
		std::string const & GetTableName () const { return _table; }
		bool IsSystemTable () const { return _isSystemTable; }
		void SetTableName (std::string const & name, bool isSystemTable) 
		{
			_table = name;
			_isSystemTable = isSystemTable;
		}
	private:
		bool		_isSystemTable;
		std::string _table;
		std::unique_ptr<Node> _predicate;
	};

	class InsertCommand: public Command
	{
	public:
		typedef NocaseMap<std::string>::const_iterator iterator;
	public:
		InsertCommand (std::string const & tableName, bool isSystemTable)
			: Command (tableName, isSystemTable)
		{}
		Type GetType () const { return Insert; }
		TuplesMap & GetTuples () { return _tuples; }
		TuplesMap const & GetTuples () const { return _tuples; }
	private:
		TuplesMap _tuples;
	};

	class FromCommand: public Command
	{
	public:
		FromCommand (std::string const & tableName, bool isSystemTable)
			: Command (tableName, isSystemTable)
		{}
		Type GetType () const { return From; }
		void Dump (std::ostream & out);
	};

	class SelectCommand: public Command
	{
	public:
		SelectCommand (std::string const & tableName, bool isSystemTable)
			: Command (tableName, isSystemTable)
		{}
		SelectCommand () {}
		Type GetType () const { return Select; }
		std::vector<std::string> & GetSelectFields () { return _fields; }
		std::vector<std::string> const & GetSelectFields () const { return _fields; }
		void AddSort (std::string const & field, bool isDescending)
		{
			_sortField = field;
			_isDescSort = isDescending;
		}
		bool HasSort () const { return !_sortField.empty (); }
		std::string const & GetSortField () const { return _sortField; }
		bool IsSortDescending () const { return _isDescSort; }
	private:
		std::vector<std::string> _fields;
		std::string _sortField;
		bool		_isDescSort;
	};

	// SQL Tables
	class Listing
	{
	public:
		class RecordFile
		{
		public:
			RecordFile (std::string const & fileName, FileTime time)
				: _fileName (fileName), _time (time)
			{}
			RecordFile (std::string const & fullPath);
			FileTime GetFileTime () const { return _time; }
			std::string GetRecordName () const { return _fileName.substr (0, _fileName.size () - 5); }
			std::string const & GetFileName () const { return _fileName; }
		private:
			std::string _fileName;
			FileTime	_time;
		};
		typedef std::vector<RecordFile>::const_iterator FileIterator;
		typedef std::vector<RecordFile>::const_reverse_iterator RevFileIterator;
	public:
		Listing (FilePath const & curPath,
				FilePath const & sysPath,
				std::string const & tableName,
				bool isSystemTable);
		std::string const & GetTableName () const { return _tableName; }
		unsigned NextRecordId () const;
		void DefaultSort (); // sort by time
		FileIterator beginFile () const { return _files.begin (); }
		FileIterator endFile () const { return _files.end (); }
		RevFileIterator rbeginFile () const { return _files.rbegin (); }
		RevFileIterator rendFile () const { return _files.rend (); }
	protected:
		FilePath	_path;
		std::string _tableName;
		std::vector<RecordFile> _files;
	};

	class Record
	{
		friend class RecordParser;
	public:
		Record (Listing::RecordFile const * file)
			: _file (file)
		{}
		Listing::RecordFile const * GetRecordFile () const { return _file; }
		TuplesMap & GetTuples () { return _tuples; }
		TuplesMap const & GetTuples () const { return _tuples; }
		bool IsLess (Record const & rec, std::string const & fieldName) const;
	protected:
		Listing::RecordFile const * _file;
		TuplesMap	_tuples;
	};

	class ExistingRecord: public Record
	{
	public:
		ExistingRecord (std::string const & fullPath, Listing::RecordFile const & recordFile);
	};

	class Table: public Listing
	{
	public:
		typedef auto_vector<Record>::const_iterator Iterator;
	public:
		Table (FilePath const & path,
			FilePath const & sysPath,
			Sql::SelectCommand const & cmd);
		Iterator begin () const { return _records.begin (); }
		Iterator end () const { return _records.end (); }
	private:
		void Sort (std::string const & fieldName, bool isDescending);
	private:
		auto_vector<Record> _records;
	};

	class WritableRecord
	{
		friend class RecordParser;
	public:
		WritableRecord (std::string const & path);
		void Update (TuplesMap & inTuples);
		void Save ();
	private:
		std::string		_path;
		std::string		_preamble;
		std::string		_trailer;
		TuplesVector	_tuples;
	};
}
#endif
