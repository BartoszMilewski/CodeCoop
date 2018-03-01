#if !defined (HUBIDDLG_H)
#define HUBIDDLG_H
// ----------------------------------
// (c) Reliable Software, 2002 - 2005
// ----------------------------------

#include <Win/Dialog.h>
#include <Ctrl/ComboBox.h>

class HubIdCtrl : public Dialog::ControlHandler
{
public:
	HubIdCtrl (std::vector<std::string> const & emailList, std::string & hubId, bool isHub);

	bool OnInitDialog () throw (Win::Exception);
    bool OnApply () throw ();

private:
	Win::ComboBox _combo;
	
	std::vector<std::string> const & _emailList;
	std::string & _hubId;
};

#endif
