//----------------------------------
//  (c) Reliable Software, 2005
//----------------------------------
#include "precompiled.h"
#include "BranchView.h"
#include <Win/WinClass.h>
#include <Win/WinMaker.h>
#include <Win/Controller.h>

class BranchViewCtrl: public Win::Controller
{
	bool MustDestroy () throw ()
		{ return true; }
};


#include "OutputSink.h"

BranchView::BranchView (Win::Dow::Handle winParent, int id)
{
	try
	{
		Win::Class::Maker classMaker ("BranchViewClass", winParent.GetInstance ());
		classMaker.Register ();
		Win::ChildMaker maker ("BranchViewClass", winParent, id);
		std::unique_ptr<Win::Controller> pCtrl (new BranchViewCtrl);
		SimpleControl::Reset (maker.Create (std::move(pCtrl)));
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
}
