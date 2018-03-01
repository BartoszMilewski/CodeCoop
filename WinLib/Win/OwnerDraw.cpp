// (c) Reliable Software 2002
#include <WinLibBase.h>
#include <Win/OwnerDraw.h>
#include <Win/Controller.h>

void OwnerDraw::Handler::Attach (Win::Dow::Handle winParent, unsigned ctrlId)
{
	_winParent = winParent;
	_ctrlId = ctrlId;
	if (!_winParent.IsNull () && _ctrlId != -1)
	{
		_winParent.GetController ()->AddDrawHandler (this);
	}
}
