//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------
#include "precompiled.h"
#include "TabCtrl.h"
#include "UiDifferIds.h"

FileTabController::FileTabController (Win::Dow::Handle win, FileViewSelector & selector)
	: Notify::TabHandler (DIFFER_TAB_HANDLER_ID),
	  _view (win, DIFFER_TAB_HANDLER_ID),
	  _selector (selector)
{
}

bool FileTabController::OnSelChange () throw ()
{
	_selector.ChangeFileView (_view.GetSelection ());
	return true;
}
