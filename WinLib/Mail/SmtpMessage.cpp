// ----------------------------------
// (c) Reliable Software, 2005 - 2006
// ----------------------------------
#include <WinLibBase.h>
#include "SmtpMessage.h"
#include <Mail/SmtpUtils.h>
#include <TimeStamp.h>
#include <Sys/SysTime.h>
#include <sstream>

const char Smtp::Message::MimeVersion [] = "MIME-Version: 1.0";

Smtp::Message::Message (std::string const & nameFrom, // actual name, like "Chad Codewell"
						std::string const & addrFrom, // email address, like "chadc@microsoft.com"
						std::string const & subject)
{
	Assert (!addrFrom.empty ());
	Assert (!nameFrom.empty ());
	_from = "From:\"";
	_from += nameFrom;
	_from += "\" ";
	_from += ToBracketedString (addrFrom);
	if (!subject.empty ())
		_subject = "Subject:" + subject;
}

void Smtp::Message::SetToField (std::vector<std::string> const & addrVector)
{
	_to = "To:";
	for (unsigned int i = 0; i < addrVector.size (); ++i)
	{
		std::string bracketedAddr = ToBracketedString (addrVector [i]);
		_to += bracketedAddr;
		if (i < addrVector.size () - 1)
			_to += ", ";
	}
}

void Smtp::Message::Send (Socket & socket)
{
	// From: required
	socket.SendLine (_from);
	// To: 
	for (LineBreakingSeq lineSeq (_to);	!lineSeq.AtEnd (); lineSeq.Advance ())
	{
		socket.SendLine (lineSeq.GetLine ());
	}
	// Subject:
	for (LineBreakingSeq lineSeq (_subject); !lineSeq.AtEnd ();	lineSeq.Advance ())
	{
		socket.SendLine (lineSeq.GetLine ());
	}
	// Date: required;
	SmtpCurrentTime currentTime;
	std::string date = "Date: " + currentTime.GetString ();
	socket.SendLine (date);
	socket.SendLine (MimeVersion);

	// Body
	_body->Send (socket);

	// End message
	socket.SendLine (".");
}
