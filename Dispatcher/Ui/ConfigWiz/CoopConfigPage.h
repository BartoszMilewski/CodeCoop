#if !defined (COOPCONFIGPAGE_H)
#define COOPCONFIGPAGE_H
// ----------------------------------
// (c) Reliable Software, 1999 - 2008
// ----------------------------------

#include "BaseWizardHandler.h"
#include "ConfigDlgData.h"

#include <Ctrl/Button.h>

class ConfigDlgData;

class CoopConfigHandler : public BaseWizardHandler
{
public:
	CoopConfigHandler (ConfigDlgData & config, unsigned pageId, bool isFirstPage)
		: BaseWizardHandler (config, pageId, isFirstPage ? PropPage::Wiz::Next : PropPage::Wiz::NextBack)
	{}
    bool OnInitDialog () throw (Win::Exception);

protected:
	void RetrieveData (bool acceptPage) throw (Win::Exception);
	long  ChooseNextPage () const throw (Win::Exception);

private:
	Win::RadioButton	_standalone;
	Win::RadioButton	_onlyLAN;
	Win::RadioButton	_onlyEmail;
	Win::RadioButton	_emailAndLAN;
};

#endif
