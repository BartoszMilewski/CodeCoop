//---------------------------
// (c) Reliable Software 2010
//---------------------------
#include "precompiled.h"
#undef DBG_LOGGING
#define DBG_LOGGING true
#include "EmailChecker.h"
#include "EmailMan.h"
#include "Email.h"
#include "Processor.h"
#include "ScriptSubject.h"
#include "DispatcherMsg.h"
#include "AlertMan.h"
#include "FeedbackMan.h"

#include <File/SafePaths.h>
#include <Win/Message.h>
#include <Mail/Pop3.h>

void EmailChecker::Run()
{
	bool verbose = false;
	if (TheEmail.GetEmailConfig ().IsAutoReceive())
		while (RetrieveEmail(verbose))
			continue;

	while (true)
	{
		unsigned timeout = Win::Event::NoTimeout;
		if (TheEmail.GetEmailConfig ().IsAutoReceive())
			timeout = TheEmail.GetEmailConfig ().GetAutoReceivePeriod ();

		verbose = _sync.Wait(timeout); // Returns true when signalled, false for timeout

		if (IsDying ())
			return;

		while (RetrieveEmail(verbose))
			continue;
	}
}

// return false if no more messages to process
bool EmailChecker::RetrieveEmail (bool isVerbose)
{
	try
	{
		dbg << "RETRIEVE EMAIL" << std::endl;
		ActivityIndicator retrieving (TheFeedbackMan, RetrievingEmail);
		std::vector<std::string> accountList;

		TheEmail.GetIncomingAccountList (accountList);
		for (std::vector<std::string>::const_iterator account = accountList.begin ();
				account != accountList.end ();
				++account)
		{
			try
			{
				dbg << "	Account: " << *account << std::endl;
				// Loop until ONE message processed
				for (InboxIterator reader (TheEmail, *account, true); // only unread messages
					!reader.AtEnd (); 
					reader.Advance ())
				{
					bool isMsgCorrupted = false;
					try
					{
						SafePaths attPaths;
						Subject::Parser subject (reader.GetSubject ());
						if (subject.IsScript ())
						{
							dbg << "    -> found script" << std::endl;
							bool isSuccess = true;
							// Save attachments in temporary area and initialize paths
							reader.RetrieveAttachements (attPaths);
							SafePaths::iterator it = attPaths.begin ();
							for (; it != attPaths.end (); ++it)
							{
								if (!UnpackFile (*it, subject.GetExtension (), reader.GetSubject (), _publicInboxPath))
									isSuccess = false;
							}

							if (isSuccess)
							{
								bool canContinue = reader.DeleteMessage ();
								Win::UserMessage msg (UM_REFRESH_VIEW);
								_winParent.PostMsg (msg);
								dbg << "RETRIEVE EMAIL: done with message!" << std::endl;
								if (!canContinue)
									return true; // <-- done with one message
							}
							else
							{
								dbg << "RETRIEVE EMAIL: unpacking failed!" << std::endl;
							}
							// attachments deleted automatically
						}
						else
						{
							// non-Co-op messages 
							if (reader.IsDeleteNonCoopMsg ())
							{
								if (!reader.DeleteMessage ())
									return true;
							}
						}
						// Good place to check again if we are not dying
						// or that the user switched off auto-receive while we were doing automatic email check
						if (IsDying () || (!TheEmail.GetEmailConfig ().IsAutoReceive() && !isVerbose))
							return false;
					}
					catch (Pop3::MsgCorruptException e)
					{
						TheAlertMan.PostInfoAlert (e, isVerbose);
						isMsgCorrupted = true;
					}

					if (isMsgCorrupted)
					{
						reader.CleanupMessage ();
					}
				}
				dbg << "    Email: Done Iterating" << std::endl;
			}
			catch (Win::SocketException e)
			{
				TheAlertMan.PostInfoAlert (e, isVerbose);
				TheEmail.ShutDown ();
			}
			catch (Pop3::Exception e)
			{
				TheAlertMan.PostInfoAlert (e, isVerbose);
				TheEmail.ShutDown ();
			}
			catch (Win::Exception e)
			{
				TheAlertMan.PostInfoAlert (e);
				TheEmail.ShutDown ();
			}
			catch ( ... )
			{
				Win::ClearError ();
				std::string msg = "Unknown error during e-mail retrieval from\n";
				msg += *account;
				TheAlertMan.PostInfoAlert (msg, isVerbose);
				TheEmail.ShutDown ();
			}
		}
	}
	catch (...)
	{
		Win::ClearError ();
		TheAlertMan.PostInfoAlert ("Unknown error during e-mail retrieval.", isVerbose);
	}
	dbg << "RETRIEVE EMAIL: nothing to do" << std::endl;
	return false; // nothing to retrieve
}

bool EmailChecker::UnpackFile (
	std::string const & fromPath,
	std::string const & extension,
	std::string const & emailSubject,
	FilePath const & mailDest)
{
	try
	{
		ScriptProcessorConfig processorCfg = TheEmail.GetScriptProcessorConfig ();
		ScriptProcessor processor (processorCfg);
		return processor.Unpack (fromPath, mailDest, extension, emailSubject);
	}
	catch ( ... )
	{
		Win::ClearError ();
		throw Win::Exception ("E-mail message with corrupted script attachment.\n"
							  "Delete it (or mark as \"Read\") in your e-mail program's inbox.\n"
							  "Ask for the script to be re-sent to you",
							   emailSubject.c_str ());
	}
}


