#if !defined PAGETAB_H
#define PAGETAB_H
//------------------------------------
//  (c) Reliable Software, 1996 - 2006
//------------------------------------

#include "DisplayMan.h"

#include <Ctrl/Tabs.h>
#include <Graph/ImageList.h>

class PageTabs: public EnumeratedTabs<ViewPage>
{
public:
    PageTabs (Win::Dow::Handle hwndParent, int id);
	~PageTabs ();
	void SetPageImage (ViewPage page, int imageIdx);
	void RemovePageImage (ViewPage page);
private:
	ImageList::AutoHandle _images;
};

class TabSequencer
{
public:
	TabSequencer (Tab::Handle const & tabs)
		: _tabs (tabs), _idx (0)
	{}
	bool AtEnd () const
	{
		return (_idx >= _tabs.GetCount ());
	}
	ViewPage GetPage () const
	{
		return _tabs.GetParam<ViewPage> (_idx);
	}
	void Advance ()
	{
		++_idx;
	}
private:
	int _idx;
	Tab::Handle const & _tabs;
};

#endif
