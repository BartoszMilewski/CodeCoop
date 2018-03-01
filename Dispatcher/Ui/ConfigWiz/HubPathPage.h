#if !defined (HUBPATHPAGE_H)
#define HUBPATHPAGE_H
// ----------------------------------
// (c) Reliable Software, 1999 - 2008
// ----------------------------------

#include "BaseWizardHandler.h"
#include "ConfigDlgData.h"
#include "resource.h"
#include <Ctrl/Button.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Static.h>
#include <File/Path.h>

class SatelliteTransportsHandler : public BaseWizardHandler
{
public:
	SatelliteTransportsHandler (ConfigDlgData & configDlgData, unsigned pageId)
		: BaseWizardHandler (configDlgData, pageId) 
	{}

    bool OnInitDialog () throw (Win::Exception);
    bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);

protected:
	void RetrieveData (bool acceptPage) throw (Win::Exception);
	bool Validate () const throw (Win::Exception);

	long ChooseNextPage () const  throw () { return IDD_WIZARD_FINISH; }

private:
    Win::Edit		_pathEdit;
	Win::Button		_pathBrowse;
	Win::Edit		_hubIdEdit;
};

class HubTransportsHandler : public BaseWizardHandler
{
public:
	HubTransportsHandler (ConfigDlgData & configDlgData, unsigned pageId)
		: BaseWizardHandler (configDlgData, pageId) 
	{}

    bool OnInitDialog () throw (Win::Exception);

protected:
	void RetrieveData (bool acceptPage) throw (Win::Exception);
	bool Validate (Win::Dow::Handle win) const throw (Win::Exception);

	long ChooseNextPage () const  throw () { return IDD_WIZARD_FINISH; }

private:
	Win::Edit		_hubIdEdit;
	Win::StaticText	_hubShare;
};


#endif
