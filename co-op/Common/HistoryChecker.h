#if !defined (HISTORYCHECKER_H)
#define HISTORYCHECKER_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

#include "GlobalId.h"

class Catalog;
namespace History
{
	class Db;
}

class HistoryChecker
{
public:
	HistoryChecker (History::Db const & history)
		: _history (history)
	{}

	GlobalId IsRelatedProject (int projectId) const;
	void DisplayError (Catalog & catalog, int thisProjectId, int selectedProjectId) const;

private:
	History::Db const &	_history;
};

#endif
