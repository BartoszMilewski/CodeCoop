// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------
#include "precompiled.h"
#include "WinOut.h"
#include "TestGlobal.h"
#include "Registry.h"

#include <Net/Socket.h>
#include <Mail/MsgTree.h>
#include <Mail/MsgParser.h>
#include <Mail/Pop3.h>
#include <Mail/Pop3Message.h>
#include <File/SafePaths.h>
#include <File/MemFile.h>
#include <Win/WinResource.h>
#include "resource.h"

char const TestName [] = "POP3/SMTP Configuration Tool";

void RunConfigTool (WinOut & out)
{
	std::string const introMsg = 
		"This test applet will lead you through the configuration of"
		"\ndirect POP3/SMTP support in Dispatcher."
		"\n\nYou will be asked for detailed settings of POP3/SMTP servers."
		"\nIt may be helpful to read them from your e-mail client program.";
	// Revisit: use TheOutput
	::MessageBox (0, introMsg.c_str (), TestName, MB_ICONINFORMATION | MB_OK);

	std::string const scriptSubject	= "Code Co-op Sync:znc.snc::RS POP3 Test:0-0";
	std::string const originalAtt	= "testOrg.znc";	
	CurrentFolder curFolder;
	std::string const originalAttFullPath (curFolder.GetFilePath (originalAtt));
	SafePaths originalAttFile;
	{
		Resource<char> resAtt (0, IDR_ACK_SCRIPT, "BINARYDATA");
		if (!resAtt.IsOk ())
			throw Win::InternalException ("Cannot find a test script in the application resources.");

		char const * attBuffer = resAtt.Lock ();
		unsigned long attSize = resAtt.GetSize ();
  		MemFileNew attFile (originalAttFullPath, File::Size (attSize, 0));
		std::memcpy (attFile.GetBuf (), attBuffer, attSize);
		attFile.Flush ();
		originalAttFile.Remember (originalAttFullPath);
	}

	// collect account data
	std::string pop3Server, pop3User, pop3Pass;
	std::string senderAddress, smtpServer, smtpUser, smtpPass;
	unsigned long options = 0;
	if (!GetAccountInfoFromUser (out, senderAddress, pop3Server, pop3User, pop3Pass, 
								 smtpServer, smtpUser, smtpPass, options))
	{
		return;
	}
	std::string const closeClientMsg = "Please, exit your e-mail program now."
									   "\nIt may interfere with the further part of the test.";
	::MessageBox (0, closeClientMsg.c_str (), TestName, MB_ICONINFORMATION | MB_OK);

	// send a test message
	{
		std::string text = "This is a test message sent by Dispatcher's POP3/SMTP configuration tool.";

		std::vector<std::string> addrVector;
		addrVector.push_back (senderAddress);

		out.PutBoldLine ("Sending a Co-op test script.");
		SendTestMessage (out, addrVector, smtpServer, smtpUser, smtpPass, senderAddress, scriptSubject, text, originalAttFullPath);
	}	
	// receive a test message
	{
		out.PutBoldLine ("The test is checking POP3 server for the Co-op test script (5 trials every 15 sec).");
		WinSocks socks;
		bool isFoundMsg = false;
		bool isValidMsg = false;
		for (unsigned int trial = 0; trial < 5 && !isFoundMsg; ++trial)
		{
			out.PutLine ("Next trial in 15 sec.");
			Win::Sleep (15 * 1000);
			out.PutLine ("Connecting to Pop3 server.");
			Pop3::Connection connection (pop3Server, 110, false);
			out.PutLine ("Logging in.");
			bool useSSL = false;
			Pop3::Session session (connection, pop3User, pop3Pass, useSSL);
			out.PutLine ("Searching for the Co-op test script on the server.");
			for (Pop3::MessageRetriever retriever (session); 
				!retriever.AtEnd () && !isFoundMsg;
				retriever.Advance ())
			{
				Pop3::Message msgHdr; // sink for the parser
				Pop3::Parser parser;
				parser.Parse (retriever.RetrieveHeader (), msgHdr);
				if (msgHdr.GetSubject () == scriptSubject)
				{
					isFoundMsg = true;
					out.PutLine ("Found the Co-op test script on the server.");
					SafePaths attPaths;
					out.PutLine ("Retrieving the script from the server.");
					Pop3::Message msg;
					parser.Parse (retriever.RetrieveMessage (), msg);
					out.PutLine ("Saving the Co-op test script on disk.");
					TmpPath tmpFolder;
					msg.SaveAttachments (tmpFolder, attPaths);
					out.PutLine ("Deleting the test script from the server.");
					retriever.DeleteMsgFromServer ();
					if (attPaths.IsEmpty ())
					{
						out.PutLine ("The received Co-op test script does not contain any attachments.");
					}
					else if (attPaths.size () != 1)
					{
						out.PutLine ("The received Co-op test script contains more than one attachments.");
					}
					else
					{
						out.PutLine ("Comparing the received script with the original script.");
						if (File::IsContentsEqual (originalAttFullPath.c_str (), attPaths.begin ()->c_str ()))
						{
							out.PutLine ("The received script is identical to the original.");
							isValidMsg = true;
						}
						else
						{
							out.PutLine ("The received script is different from the original script.");
						}
					}
				}
			}
		}
		if (isFoundMsg && isValidMsg)
			out.PutLine ("The SMTP/POP3 test completed successfully.");
		else
		{
			out.PutLine ("The SMTP/POP3 test failed.");
			return;
		}
#if 0
		// test successful, ask user to turn on POP3/SMTP
		std::string const usePop3Msg = "Do you want to use POP3 in Dispatcher?";
		Registry::UserDispatcherPrefs prefs;
		bool startUsing = ::MessageBox (0, usePop3Msg.c_str (), TestName, MB_ICONQUESTION | MB_YESNO) == IDYES;
		prefs.SetUsePop3 (startUsing);
		if (startUsing)
		{
			std::string pop3Info = "You have chosen to use POP3 in Dispatcher."
				"\nThe setting will take an effect next time you run Dispatcher."
				"\n\nIf you do not have a separate e-mail account for Co-op scripts and"
				"\nyou are still going to check for new messages using your e-mail program,"
				"\nincrease the value of \"Check for new messages every [XX] minutes\" option"
				"\nin your e-mail program."
				"\n\nYou can also select \"Leave messages on the server\" option to make sure"
				"\nDispatcher will have a chance to retrieve all scripts fro the server.";
			::MessageBox (0, pop3Info.c_str (), TestName, MB_ICONINFORMATION | MB_OK);
		}
		std::string const useSmtpMsg = "Do you want to use SMTP in Dispatcher?";
		startUsing = ::MessageBox (0, useSmtpMsg.c_str (), TestName, MB_ICONQUESTION | MB_YESNO) == IDYES;
		prefs.SetUseSmtp (startUsing);
#endif
	}
}
