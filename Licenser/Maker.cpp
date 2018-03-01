//----------------------------------
// (c) Reliable Software 2005 - 2006
//----------------------------------
#include "precompiled.h"
#include "Maker.h"
#include "LicenseRequest.h"
#include "DistributorScriptSend.h"
#include "Address.h"
#include "Crypt.h"
#include "Email.h"
#include "EmailMessage.h"
#include "LocalMailer.h"

#include <Mail/EmailAddress.h>
#include <File/MemFile.h>
#include <fstream>

std::string const CoopSeats    = "&COOP_SEATS;";
std::string const BCSeats      = "&BC_SEATS;";
std::string const Price        = "&PRICE;";
std::string const LicenseeName = "&LICENSEE_NAME;";
std::string const LicenseKey   = "&LICENSE_KEY;";

void find_replace (std::string & text, std::map<std::string, std::string> const & markerMap)
{
	std::string::size_type current = text.find ('&');
	if (current == std::string::npos)
		return;

	std::string::const_iterator firstNotAdded = text.begin ();
	do
	{
		Assert (current != std::string::npos);
		// find semicolon
		std::string::size_type semicolonPos = text.find (';', current + 1);
		if (semicolonPos == std::string::npos)
			break;

		std::string marker = text.substr (current, semicolonPos - current + 1);
		std::map<std::string, std::string>::const_iterator markerIter = markerMap.find (marker);
		if (markerIter == markerMap.end ())
			throw Win::InternalException ("Marker found in text has no associated expansion.");

		text.replace (current, marker.length (), markerIter->second);
		current += markerIter->second.length ();
		firstNotAdded = text.begin () + current;
		current = text.find ('&', current);
	} while (current != std::string::npos);
}

LicenseMaker::~LicenseMaker () {}

void LicenseMaker::SetComment (std::string const & comment)
{
	_req.Comment () = comment;
}

bool SimpleLicenseMaker::IsValid ()
{


return true;



	if (_req.TemplateFile ().empty ())
	{
		_req.Comment () = "Please provide a template file for the confirmation message body";
		return false;
	}
	if (!Email::IsValidAddress(_req.Email ()))
	{
		_req.Comment () = "Please provide valid email address of the recipient of license(s)";
		return false;
	}
	if (_req.Licensee ().empty ())
	{
		_req.Comment () = "Please provide licensee name";
		return false;
	}
	if (_req.Seats () <= 0 || _req.Seats () >= 1000)
	{
		_req.Comment () = "Please provide seat count > 0 and < 1000";
		return false;
	}
	if (_req.Price () == 0)
	{
		_req.Comment () = "Please provide price";
		return false;
	}
	return true;
}

bool DistributorLicenseMaker::IsValid ()
{
	if (!Email::IsValidAddress (_req.Email ()))
	{
		_req.Comment () = "Please provide valid email address of the recipient of license(s)";
		return false;
	}
	if (_req.Licensee ().empty ())
	{
		_req.Comment () = "Please provide licensee name";
		return false;
	}
	if (_req.StartNum () < 0 || _req.StartNum () > 1000)
	{
		_req.Comment () = "Please provide valid starting serial number between 0 and 1000";
		return false;
	}
	if (_req.LicenseCount () <= 0 || _req.LicenseCount () >= 1000)
	{
		_req.Comment () = "Please provide license count > 0 and < 1000";
		return false;
	}
	return true;
}

bool BlockLicenseMaker::IsValid ()
{
	if (_req.LicenseCount () <= 0 || _req.LicenseCount () >= 1000)
	{
		_req.Comment () = "Please provide valid number of reseller licenses > 0 and < 1000";
		return false;
	}
	if (_req.StartNum () < 10000 || _req.StartNum () > 99999)
	{
		_req.Comment () = "Please provide valid starting serial number between 10000 and 99999";
		return false;
	}
	if (_req.Seats () <= 0 || _req.Seats () >= 1000)
	{
		_req.Comment () = "Please provide seat count > 0 and < 1000";
		return false;
	}
	return true;
}

void SimpleLicenseMaker::Make (FilePath const & workFolder)
{
	std::map <std::string, std::string> markerExpansionMap;	
	markerExpansionMap [CoopSeats]    = ToString (_req.Seats ());
	markerExpansionMap [BCSeats]      = ToString (_req.BCSeats());
	markerExpansionMap [Price]	      = ToString (_req.Price ());
	markerExpansionMap [LicenseeName] = _req.Licensee ();
	markerExpansionMap [LicenseKey]   = EncodeLicense10 (_req.Licensee (), _req.Seats (), _req.Product ());

	// load form letter template
	std::string formLetter;
    if (_req.TemplateFile() != "")
	{
		MemFileReadOnly formLetterFile (workFolder.GetFilePath (_req.TemplateFile ()));
		formLetter.assign (formLetterFile.GetBuf (), formLetterFile.GetBufSize ());
	}
    else
    {
        formLetter = "This email confirms your order for a &COOP_SEATS;-seat license of Code Co-op.\n"
            "Input the license information exactly as shown below.\n"
            "Licensee name : &LICENSEE_NAME;\n"
            "License key : &LICENSE_KEY;\n";
    }
	// customize template
	find_replace (formLetter, markerExpansionMap);

	// create mail message
	LocalMailer mailer;
	OutgoingMessage mailMsg;
	mailMsg.SetSubject ("Code Co-op License Purchase Confirmation");
	// add purchase receipt attachment
	// mailMsg.AddFileAttachment (workFolder.GetFilePath ("PurchaseReceipt.pdf"));
	mailMsg.SetText (formLetter);
	if (_req.Preview ())
		mailer.Save (mailMsg, _req.Email ());
	else
		mailer.Send (mailMsg, _req.Email ());

	_req.Comment () = "License sent successfully!";
}

void DistributorLicenseMaker::Make (FilePath const & workFolder)
{
	// load instructions from a file
	std::string instructions;
	{
		MemFileReadOnly instructionsFile (workFolder.GetFilePath ("DistributorInstructions.txt"));
		instructions.assign (instructionsFile.GetBuf (), instructionsFile.GetBufSize ());
	}

	MailOrSaveDistributorBlock (
		_req.Email (),
		DispatcherAtRelisoft, 
		"support@ReliSoft.com",
		_req.Licensee (), _req.StartNum (), _req.LicenseCount (),
		instructions,
		_req.Preview ());

	std::string & comment = _req.Comment ();
	comment = "Distribution license block:\n\nLicensee: ";
	comment += _req.Licensee ();
	comment += "\nStarting number: ";
	comment += ToString (_req.StartNum ());
	comment += "\nLicense count: ";
	comment += ToString (_req.LicenseCount ());
	if (_req.Preview ())
		comment += "\n\n will be sent to: ";
	else
		comment += "\n\nwas sent to: ";
	comment += _req.Email ();
}
	
void  BlockLicenseMaker::Make (FilePath const & workFolder)
{
	std::ostringstream block;
	for (int i = 0; i < _req.LicenseCount (); ++i)
	{
		std::string licensee = ToString (_req.StartNum () + i);
		block << licensee << "\t" << EncodeLicense10 (licensee, _req.Seats (), _req.Product ()) << std::endl;
	}
	OutStream blockFile (workFolder.GetFilePath ("Block.txt"));
	if (!blockFile)
		throw Win::InternalException ("Could not open destination file.", workFolder.GetFilePath ("Block.txt"));
	blockFile << block.str ();
	_req.Comment () = "Block of licenses saved successfully!";
}
