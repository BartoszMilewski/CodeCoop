#if !defined (FINISHPAGE_H)
#define FINISHPAGE_H
// ---------------------------------
// (c) Reliable Software 1999 - 2008
// ---------------------------------

#include "BaseWizardHandler.h"
#include "resource.h"

class ConfigDlaData;

class FinishHandler : public BaseWizardHandler
{
public:
	FinishHandler (ConfigDlgData & configData, unsigned pageId)
		: BaseWizardHandler (configData, pageId, PropPage::Wiz::BackFinish)
	{}
};

#endif
