//---------------------------
// Generator.cpp
// (c) Reliable Software 2000
//---------------------------

#include "HeaderDetails.h"

#include "Catalog.h"
#include "TransportHeader.h"
#include "DispatcherScript.h"
#include "Addressee.h"
#include "Serialize.h"

#include <Dbg/Assert.h>

void GenerateScript (HeaderDetails const & details)
{
	// Interpret sheet results
	FilePath destPath;
	switch (details._destFolder)
	{
	case HeaderDetails::PublicInbox:
	{
		Catalog cat;
		cat.GetPublicInboxDir (destPath);
		break;
	}
	case HeaderDetails::PrivateInbox:
		Assert (!"Not implemented yet");
		break;
	case HeaderDetails::PrivateOutbox:
		Assert (!"Not implemented yet");
		break;
	case HeaderDetails::UserDefined:
		destPath.Change (details._destPath);
		break;
	};

	// Initiate TransportHeader
	TransportHeader txHdr;
	txHdr.AddSender (details._projectName.c_str (), 
					 details._senderEmail.c_str (), 
					 details._senderId.c_str ());
	txHdr.SetDefect (details._isDefect);
	txHdr.SetDispatcherAddendum (details._hasAddendums);
	txHdr.SetForward (details._toBeForwarded);
	AddresseeList addressees;
	addressees.Add (Addressee (details._recipEmail, details._recipId));
	txHdr.AddAddressees (addressees);

	// Initiate DispatcherScript (strange name; it contains addendums)
	DispatcherScript addendums;
//	std::auto_ptr<DispatcherCmd> fwdPathChange (new FwdPathChangeCmd (thisUser->GetEmail(),
//																 thisUser->GetLocation (),
//																 publicInboxShare));
//	dispatcherScript.AddCmd (fwdPathChange);

	// Save script
	char const * path = destPath.GetFilePath (details._scriptFilename);
	FileSerializer out (path);
	txHdr.Save (out);
	if (addendums.CmdCount () != 0)
		addendums.Save (out);
}
