#if !defined (UPDATEHUBIDDLG_H)
#define UPDATEHUBIDDLG_H
//------------------------------------
//  (c) Reliable Software, 2002 - 2005
//------------------------------------

#include <Win/Dialog.h>
#include <Ctrl/Edit.h>

class UpdateHubIdCtrl : public Dialog::ControlHandler
{
public:
	UpdateHubIdCtrl (std::string const & catalogHubId, std::string const & userAddress);

    bool OnInitDialog () throw (Win::Exception);
    bool OnApply () throw ();

private:
	Win::EditReadOnly	_hubId;
	Win::EditReadOnly	_address;
	std::string const &	_catalogHubId;
	std::string const &	_userAddress;
};

#endif
