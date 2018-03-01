#if !defined (TAB_H)
#define TAB_H
// --------------------------------
// (c) Reliable Software, 2001 - 06
// --------------------------------

#include <Ctrl/Tabs.h>
#include <Win/Win.h>
#include <Graph/ImageList.h>

enum ViewType;

class MainWinTabs : public EnumeratedTabs<ViewType>
{
public:
	MainWinTabs (Win::Dow::Handle hwndParent, int id);
	~MainWinTabs ();

	void SetPageImage (ViewType page, int imageIdx);
	void RemovePageImage (ViewType page);
private:
	ImageList::AutoHandle _images;
};

#endif
