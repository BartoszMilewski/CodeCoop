#if !defined (VIEW_H)
#define VIEW_H
// ---------------------------
// (c) Reliable Software, 2001
// ---------------------------

#include <Ctrl/ListView.h>
#include <Win/Win.h>

class ItemView : public Win::ReportListing
{
public:
	ItemView (Win::Dow::Handle winParent, int id);
};

#endif
