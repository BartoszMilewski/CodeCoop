#if !defined (EMAIL_OPTIONS_PAGE_H)
#define EMAIL_OPTIONS_PAGE_H
// ---------------------------------
// (c) Reliable Software 1999 - 2008
// ---------------------------------

#include "BaseWizardHandler.h"
#include "resource.h"

#include <Ctrl/Button.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Spin.h>

class ConfigDlgData;

class EmailOptionsHandler : public BaseWizardHandler
{
public:
	EmailOptionsHandler (ConfigDlgData & config, unsigned pageId)
		: BaseWizardHandler (config, pageId)
	{}

    bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);

	bool Validate () const;

protected:
	void RetrieveData (bool acceptPage);
	long ChooseNextPage () const  throw () { return IDD_WIZARD_FINISH; }

private:
	Win::Edit		 _maxEmailSize;
	Win::CheckBox	 _autoReceive;
	Win::Edit		 _autoReceivePeriod;
	Win::Spin		 _autoReceiveSpin;
};

#endif
