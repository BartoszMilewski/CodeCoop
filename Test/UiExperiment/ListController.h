#if !defined (LISTCONTROLLER_H)
#define LISTCONTROLLER_H
//---------------------------
// (c) Reliable Software 2009
//---------------------------
#include <auto_vector.h>
#include <Ctrl/Header.h>
#include "HeadedList.h"


class ListController
{
public:
	ListController(Win::Dow::Handle winParent)
		: _winParent(winParent)
	{
		Win::HeaderMaker hdrMaker(_winParent, 0);
		_header.Reset(hdrMaker.Create(_winParent));
		int item1 = _header.AppendItem("File Name", 200);
		_header.AppendItem("Size", 100);

		std::auto_ptr<HeadedList> block1(new HeadedList(_winParent, "Change number 1"));
		_blocks.push_back(block1);
		std::auto_ptr<HeadedList> block2(new HeadedList(_winParent, "Change number 2"));
		_blocks.push_back(block2);
		std::auto_ptr<HeadedList> block3(new HeadedList(_winParent, "New Change"));
		_blocks.push_back(block3);
	}
	void MoveView(Win::Rect rect)
	{
		_header.Move(rect.Left(), rect.Top(), rect.Width(), 30);
		rect.ShiftBy(0, 30);
		for (Iterator it = _blocks.begin(); it != _blocks.end(); ++it)
		{
			int dy = (*it)->MoveView(rect);
			rect.ShiftBy(0, dy);
		}
	}
private:
	Win::Dow::Handle	_winParent;
	Win::Header			_header;
	auto_vector<HeadedList> _blocks;
	typedef auto_vector<HeadedList>::const_iterator Iterator;
};

#endif
