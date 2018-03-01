//------------------------------------
//  (c) Reliable Software, 2007 - 2008
//------------------------------------

#include "precompiled.h"
#include "DefaultEmailAddress.h"
#include "Email.h"
#include "EmailPromptCtrl.h"
#include "Prompter.h"

#include <Mail/EmailAddress.h>

std::string Email::GetDefaultAddress (Email::Manager & emailMan)
{
	std::string emailAddress;
	try
	{
		Mailer mailer (emailMan);
		std::string userName;
		mailer.GetLoggedUser (userName, emailAddress);
		if (!Email::IsValidAddress (emailAddress))
			emailAddress.clear ();
	}
	catch (...)
	{
		// ignore all errors
		emailAddress.clear ();
	}
	return emailAddress;
}
