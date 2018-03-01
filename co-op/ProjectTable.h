#if !defined (PROJECTTABLE_H)
#define PROJECTTABLE_H
//------------------------------------
//	(c) Reliable Software, 1999 - 2007
//------------------------------------

#include "Table.h"
#include "Global.h"

class FilePath;
class Catalog;

namespace Project
{

	DegreeOfInterest IsInteresting (Catalog const & catalog, int projectId);

	class Dir : public Table
	{
	public:
		Dir (Catalog & catalog)
			: _catalog (catalog),
			  _curProjId (-1)
		{}

		bool FolderChange (FilePath const & folder);
		bool IsEmpty () const;
		DegreeOfInterest HowInteresting () const;

		// Table interface

		void QueryUniqueIds (Restriction const & restrict, GidList & ids) const;
		Table::Id GetId () const { return Table::projectTableId; }
		bool IsValid () const { return true; }
		std::string GetStringField (Column col, GlobalId gid) const;
		std::string GetStringField (Column col, UniqueName const & uname) const 
		{
			return std::string ();
		}
		GlobalId GetIdField (Column col, UniqueName const & uname) const { return gidInvalid; }
		GlobalId GetIdField (Column col, GlobalId gid) const { return gidInvalid; }
		unsigned long GetNumericField (Column col, GlobalId gid) const;
		bool GetBinaryField (Column col, void * buf, unsigned int bufLen, GlobalId gid) const;

		// current project
		void Enter (int projectId) { _curProjId = projectId; }
		int  GetCurProjectId () const { return _curProjId; }
		void Exit () { _curProjId = -1; }

	private:
		Catalog &		_catalog;
		int				_curProjId;
	};
}

#endif
