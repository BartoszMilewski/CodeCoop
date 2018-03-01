//------------------------------------
//  (c) Reliable Software, 2002
//------------------------------------
#include "precompiled.h"
#include "Ctrl.h"
#include "Resource/resource.h"

UiCtrl::UiCtrl ()
	: Notify::TreeViewHandler (ID_TREE), _imageList (16, 16, 4)
{
}

UiCtrl::~UiCtrl ()
{
}

bool UiCtrl::OnCreate (Win::CreateData const * create, bool & success) throw ()
{
	Win::Dow::Handle win = GetWindow ();
	try
	{
		TheOutput.Init (0, "Ui");
		Tree::Maker treeMak (win, ID_TREE);
		treeMak.Style () << Tree::View::Style::HasLines
						<< Tree::View::Style::LinesAtRoot
						<< Tree::View::Style::HasButtons;
		_tree = treeMak.Create ();

		ImageList::Handle ;
		Icon::Maker icon (16, 16);
		_imageList.AddIcon (icon.Load (win.GetInstance (), I_FOLDER));
		_imageList.AddIcon (icon.Load (win.GetInstance (), I_FOLDERG));
		_imageList.AddIcon (icon.Load (win.GetInstance (), I_FOLDERO));
		_imageList.AddIcon (icon.Load (win.GetInstance (), I_FOLDEROG));
		
		_tree.AttachImageList (_imageList);

		Tree::Node rootItem;
		rootItem.SetText ("Root");
		rootItem.SetIcon (0, 2);
		_root = _tree.InsertRoot (rootItem);

		Tree::Node childItem (_root);
		childItem.SetText ("Child");
		childItem.SetIcon (1, 3);
		Tree::Node child = _tree.AppendChild (childItem);

		childItem.SetText ("Another child");
		childItem.SetIcon (0, 2);
		_tree.AppendChild (childItem);

		Tree::Node grandChildItem (child);
		grandChildItem.SetText ("Grandchild");
		grandChildItem.SetIcon (1, 3);
		_tree.AppendChild (grandChildItem);

		_tree.Expand (_root);
		success = true;
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
		success = false;
	}
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Display ("Initialization -- Unknown Error", Out::Error);
		success = false;
	}
	TheOutput.SetParent (win);
	return true;
}

bool UiCtrl::OnSize (int width, int height, int flag) throw ()
{
	_tree.Move (0, 0, width, height);
	return true;
}


bool UiCtrl::OnDestroy () throw ()
{
	Win::Quit ();
	return true;
}

bool UiCtrl::OnItemExpanding (Tree::View::Item & item) throw ()
{
	return true;
}
bool UiCtrl::OnItemExpanded (Tree::View::Item & item) throw ()
{
	return true;
}
bool UiCtrl::OnSelChanging (Tree::View::Item & item) throw ()
{
	return true;
}
bool UiCtrl::OnSelChanged (Tree::View::Item & item) throw ()
{
	return true;
}

bool UiCtrl::OnGetDispInfo (Tree::View::Request const & request,
				Tree::View::State const & state,
				Tree::View::Item & item) throw ()
{
	return true;
}
