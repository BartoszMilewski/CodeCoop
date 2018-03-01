#if !defined (TABCTRL_H)
#define TABCTRL_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------
#include "FileViews.h"
#include <Ctrl/Tabs.h>

class FileTabController: public Notify::TabHandler
{
public: 
	typedef EnumeratedTabs<FileSelection> View;
public:
	FileTabController (Win::Dow::Handle win, FileViewSelector & selector);

	// Notify::TabHandler
	bool OnSelChange () throw ();
	View & GetView () { return _view; }
private:
	View _view;
	FileViewSelector & _selector;
};

#endif
