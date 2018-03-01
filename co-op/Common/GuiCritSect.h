#if !defined (GUICRITSECT_H)
#define GUICRITSECT_H
// (c) Reliable Software 2002
#include <Sys/Synchro.h>

// combination of counting critical section 
// and busy bit (the base class fakes busy bit)
class GuiCritSect: public Win::CritSection
{
public:
	GuiCritSect () : _isBusy (false) {}
	bool IsBusy () const { return _isBusy; }
	bool & GetBusy () { return _isBusy; }
protected:
	bool	_isBusy;		// Reentrance lock
};

class GuiLock: public Win::Lock
{
public:
	GuiLock (GuiCritSect & critSect)
		: Win::Lock (critSect), _isBusy (critSect.GetBusy ())
	{
		_isBusy = true;
	}
	~GuiLock ()
	{
		_isBusy = false;
	}
private:
	bool & _isBusy;
};

#endif
