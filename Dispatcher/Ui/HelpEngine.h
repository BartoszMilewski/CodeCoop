#if !defined (HELPENGINE_H)
#define HELPENGINE_H
//------------------------------------------------
// HelpEngine.h
// (c) Reliable Software 2002
// -----------------------------------------------

#include <Win/Help.h>

class HelpEngine : public Help::Engine
{
public:
	HelpEngine () {}

	bool OnDialogHelp (int dlgId) throw ();

private:
	struct Item
	{
		int				_id;
		char const *	_info;
	};

private:
	static Item const _items [];
};

#endif
