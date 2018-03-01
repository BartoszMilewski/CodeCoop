#if !defined (RESTOREPAGE_H)
#define RESTOREPAGE_H
//----------------------------------
//  (c) Reliable Software, 2008
//----------------------------------

#include "BaseWizardHandler.h"
#include "resource.h"

class ConfigDlaData;

class RestoreHandler : public BaseWizardHandler
{
public:
	RestoreHandler (ConfigDlgData & configData, unsigned pageId)
		: BaseWizardHandler (configData, pageId, PropPage::Wiz::Next)
	{}

	long ChooseNextPage () const throw (Win::Exception) { return IDD_WIZARD_INTRO; }

};

#endif
