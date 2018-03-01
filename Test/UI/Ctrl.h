#if !defined (UICTRL_H)
#define UICTRL_H
//------------------------------------
//  (c) Reliable Software, 2002
//------------------------------------
#include <Win/Controller.h>
#include <Ctrl/Tree.h>
#include <Graph/ImageList.h>

extern Out::Sink TheOutput;

class UiCtrl: public Win::Controller, public Notify::TreeViewHandler
{
public:
	UiCtrl ();
	~UiCtrl ();
	bool OnCreate (Win::CreateData const * create, bool & success) throw ();
	bool OnDestroy () throw ();
	bool OnSize (int width, int height, int flag) throw ();
	// TreeView notifications
	bool OnGetDispInfo (Tree::View::Request const & request,
				Tree::View::State const & state,
				Tree::View::Item & item) throw ();
	bool OnItemExpanding (Tree::View::Item & item) throw ();
	bool OnItemExpanded (Tree::View::Item & item) throw ();
	bool OnSelChanging (Tree::View::Item & item) throw ();
	bool OnSelChanged (Tree::View::Item & item) throw ();
	Notify::Handler * GetNotifyHandler (Win::Dow::Handle winFrom, int idFrom)
	{
		if (winFrom == _tree)
			return this;
		else
			return 0;
	}
private:
	ImageList::AutoHandle _imageList;
	Tree::View _tree;
	Tree::NodeHandle _root;
};

#endif
