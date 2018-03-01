#if !defined (CHECKINAREA_H)
#define CHECKINAREA_H
//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "Table.h"
#include "Global.h"

class DataBase;
class PathFinder;
class Catalog;
namespace Project
{
	class Dir;
}
namespace CheckOut
{
	class Db;
}
class CheckInArea : public Table
{
public:
	CheckInArea (DataBase const & dataBase,
				 PathFinder & pathFinder,
				 Catalog & catalog,
				 Project::Dir & project,
				 CheckOut::Db const & checkOutDb)
		: _dataBase (dataBase),
		  _pathFinder (pathFinder),
		  _catalog (catalog),
		  _project (project),
		  _checkOutDb (checkOutDb)
	{}

	Table::Id GetId () const { return Table::checkInTableId; }
	bool IsEmpty () const;

	// Table interface

	void QueryUniqueIds (Restriction const & restrict, GidList & ids) const;
	bool IsValid () const;
	std::string GetStringField (Column col, GlobalId gid) const;
	std::string GetStringField (Column col, UniqueName const & uname) const;
	GlobalId	GetIdField (Column col, UniqueName const & uname) const;
	GlobalId	GetIdField (Column col, GlobalId gid) const;
	unsigned long GetNumericField (Column col, GlobalId gid) const;
    bool GetBinaryField (Column col, void * buf, unsigned int bufLen, GlobalId gid) const;

	std::string GetCaption (Restriction const & restrict) const;

private:
	PathFinder &			_pathFinder;
	DataBase const &		_dataBase;
	Catalog &				_catalog;
	Project::Dir &			_project;
	CheckOut::Db const &	_checkOutDb;

};

#endif
