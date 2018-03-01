//---------------------------
//    Sheet.cpp
// (c) Reliable Software 2000
//---------------------------

#include "HeaderDetails.h"
#include "GeneralPage.h"
#include "AddressPage.h"
#include "AddendumPage.h"
#include "DestinationPage.h"
#include "resource.h"

#include <Ctrl/PropertySheet.h>

bool CollectData (HINSTANCE hInst, HeaderDetails & details)
{
	Property::Sheet sheet (hInst, "Script Generator", 5);
	GeneralCtrl generalCtrl (details);
	sheet.InitPage (0, IDD_GENERAL, generalCtrl, false, "General");
	AddressCtrl senderAddressCtrl (details, false);
	sheet.InitPage (1, IDD_ADDRESS, senderAddressCtrl, false, "Sender");
	AddressCtrl recipAddressCtrl (details, true);
	sheet.InitPage (2, IDD_ADDRESS, recipAddressCtrl, false, "Recipient");
	AddendumCtrl addendumCtrl (details);
	sheet.InitPage (3, IDD_ADDENDUM, addendumCtrl, false, "Addendums");
	DestinationCtrl destinationCtrl (details);
	sheet.InitPage (4, IDD_DESTINATION, destinationCtrl, false, "Destination");
	return sheet.Run () && !details._wasCanceled;
}
