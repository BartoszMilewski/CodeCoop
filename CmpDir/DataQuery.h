#if !defined (DATAQUERY_H)
#define DATAQUERY_H
// (c) Reliable Software 2003
#include "DataPortal.h"
#include <File/Path.h>

namespace Data
{
	// Queries may be compared for equality

	class ListQuery: public Query
	{
	public:
		ListQuery (FilePath const & path)
			:_path (path)
		{}
		FilePath const & GetPath () const { return _path; }
		virtual bool operator== (Query const & query)
		{
			ListQuery const * q = dynamic_cast<ListQuery const *> (&query);
			return q != 0 && _path.IsEqualDir (q->GetPath ());
		}
	private:
		FilePath _path;
	};

	class CmpQuery: public Query
	{
	public:
		CmpQuery (FilePath const & oldPath, FilePath const & newPath)
			:_oldPath (oldPath), _newPath (newPath)
		{}
		FilePath const & GetOldPath () const { return _oldPath; }
		FilePath const & GetNewPath () const { return _newPath; }
		virtual bool operator== (Query const & query)
		{
			CmpQuery const * q = dynamic_cast<CmpQuery const *> (&query);
			return q != 0 && _oldPath.IsEqualDir (q->GetOldPath ())
						  && _newPath.IsEqualDir (q->GetNewPath ());
		}
	private:
		FilePath _oldPath;
		FilePath _newPath;
	};
}

#endif
