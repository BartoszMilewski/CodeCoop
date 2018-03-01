#if !defined HIERARCHYVIEW_H
#define HIERARCHYVIEW_H
//------------------------------------
// (c) Reliable Software 1997 -- 2002
//------------------------------------
#include <Ctrl/Tree.h>
#include <Ctrl/ListView.h>

class HierarchyView : public Tree::View
{
public:
	enum FolderImage
	{
		imgClosed = 0,
		imgOpen,
		imgGreyClosed,
		imgGreyOpen,
		imgProjClosed,
		imgProjOpen,
		imgCount // must be last
	};
    HierarchyView (Win::Dow::Handle winParent, int id);
	Tree::NodeHandle AddRoot (std::string const & name, bool hasChildren);
private:
	ImageList::AutoHandle _images;
	static int imgResources [imgCount];
};

#endif
