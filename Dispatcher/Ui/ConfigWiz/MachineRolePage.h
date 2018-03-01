#if !defined (MACHINEROLEPAGE_H)
#define MACHINEROLEPAGE_H
// ----------------------------------
// (c) Reliable Software, 1999 - 2008
// ----------------------------------

#include "BaseWizardHandler.h"
#include "resource.h"

#include <Ctrl/Button.h>

class ConfigDlgData;

class MachineRoleHandler : public BaseWizardHandler
{
public:
	MachineRoleHandler (ConfigDlgData & config, unsigned pageId)
		: BaseWizardHandler (config, pageId)
	{}

    bool OnInitDialog () throw (Win::Exception);

protected:
	void RetrieveData (bool acceptPage) throw (Win::Exception);
	long ChooseNextPage () const throw ();

private:
	Win::RadioButton	_isHub;
	Win::RadioButton	_isSat;
};

#endif
