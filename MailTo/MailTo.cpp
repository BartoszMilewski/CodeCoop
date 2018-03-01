//
// (c) Reliable Software 1998
//

#include "CmdLineArgs.h"
#include "Message.h"
#include "OutputSink.h"
#include "SimpleRecipientList.h"
#include "SimpleSession.h"
#include "SimpleAddrBook.h"
#include "SimpleStore.h"

#include <Ex/WinEx.h>
#include <Sys/WinString.h>

int PASCAL WinMain (HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdParam, int cmdShow)
{
	TheOutput.Init (0, "MailTo");

	try
	{
		CmdLineArgs args (cmdParam);
		if (args.AreOk ())
		{
			OutgoingMessage mailMsg;
			mailMsg.SetSubject ("Code Co-op Synch Scripts");
			mailMsg.SetText ("This message was sent by the Code Co-op 'MailTo' utility");
			mailMsg.AddFileAttachment (args.GetFileName ());

			RecipientList recipients (args.GetRecipients ());

			Session session;
			AddressBook addressBook (session);
			recipients.Verify (addressBook);
			
			Outbox outbox (session);
			outbox.Submit (mailMsg, recipients, args.IsVerbose ());
		}
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch (...)
	{
		LastSysErr lastErr;
		TheOutput.Display (lastErr.Text (), Out::Error);
	}
	return 0;
}