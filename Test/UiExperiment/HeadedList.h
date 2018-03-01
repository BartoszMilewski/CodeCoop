#if !defined (HEADEDLIST_H)
#define HEADEDLIST_H
//---------------------------
// (c) Reliable Software 2009
//---------------------------
#include <Ctrl/Edit.h>

class HeadedList
{
public:
	HeadedList(Win::Dow::Handle winParent, std::string title)
		: _winParent(winParent)
	{
		Win::EditMaker maker(winParent, 1);
		maker.Style() << Win::Style::AddBorder;
		_edit = maker.Create(winParent);
		_edit.SetText(title);
	}
	int MoveView(Win::Rect const & rect)
	{
		_edit.Move(rect.Left(), rect.Top(), rect.Width(), 100);
		return 100;
	}
private:
	Win::Dow::Handle	_winParent;
	Win::Dow::Handle	_edit;
};

#endif
