#if !defined (TABLEVIEWER_H)
#define TABLEVIEWER_H
//----------------------------------
// (c) Reliable Software 1997 - 2006
//----------------------------------

#include <Ctrl/ListView.h>

class TableViewer : public Win::ReportCallback
{
public:
    TableViewer (Win::Dow::Handle winParent, int id, bool isSingleSelection);
	~TableViewer ();

private:
	ImageList::AutoHandle _images;
	ImageList::AutoHandle _columnImages;
};

#endif
