//-----------------------------------------
// (c) Reliable Software 1997-2002
//-----------------------------------------

#include "precompiled.h"
#include "HierarchyView.h"
#include "Image.h"

// in the order of the enumeration FolderImage
int HierarchyView::imgResources [] = { I_FOLDERC, I_FOLDERO, I_FOLDERCG, I_FOLDEROG, 
										I_PROJCLOSED, I_PROJOPEN };

HierarchyView::HierarchyView (Win::Dow::Handle winParent, int id)
	: _images (16, 16, imageLast)
{
	Tree::Maker treeMak (winParent, id);
	treeMak.Style () << Tree::View::Style::HasLines
					<< Tree::View::Style::LinesAtRoot
					<< Tree::View::Style::HasButtons
					<< Tree::View::Style::ShowSelAlways
					<< Tree::View::Style::TrackSelect;
	SimpleControl::Reset (treeMak.Create ());

    for (int i = 0; i < imgCount; i++)
    {
		Icon::SharedMaker icon (16, 16);
		_images.AddIcon (icon.Load (winParent.GetInstance (), imgResources [i]));
    }

	AttachImageList (_images);
}

Tree::NodeHandle HierarchyView::AddRoot (std::string const & name, bool hasChildren)
{
	Tree::Node rootItem;
	rootItem.SetText (name.c_str ());
	rootItem.SetIcon (HierarchyView::imgProjClosed, HierarchyView::imgProjOpen);
	rootItem.SetHasChildren (hasChildren);
	return InsertRoot (rootItem);
}

