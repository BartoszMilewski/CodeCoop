#if !defined (SOLUTIONTOOLCOMMANDER_H)
#define SOLUTIONTOOLCOMMANDER_H
//----------------------------
// (c) Reliable Software, 2004
//----------------------------
#include <Ctrl/Command.h>
class ModelessManager;

class Commander
{
public:
	Commander (Win::Dow::Handle topWindow)
		: _topWindow (topWindow), _modelessMan (0)
	{}
	void SetModelessManager (ModelessManager * man)
	{
		_modelessMan = man;
	}
	void Program_Exit ();
	Cmd::Status can_Program_Exit () const { return Cmd::Enabled; }
	void Program_About ();
	void Dialogs_Modeless ();
	void Dialogs_Listview ();
private:
	Win::Dow::Handle  _topWindow;
	ModelessManager * _modelessMan;
};

#endif
