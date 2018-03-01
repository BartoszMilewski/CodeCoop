//
// (c) Reliable Software 1998
//

#include "SimpleStore.h"
#include "SimpleSession.h"
#include "SimpleRecipientList.h"
#include "Message.h"
#include "MapiEx.h"

#include <File/File.h>

#include <string>

class LocalTime : public SYSTEMTIME
{
public:
	LocalTime ()
	{
		::GetLocalTime (this);
	}
	std::string GetTimeStr () const;
private:
	static char const * _month [];
};

char const * LocalTime::_month [] = { "Jan",
									  "Feb",
									  "Mar",
									  "Apr",
									  "May",
									  "Jun",
									  "Jul",
									  "Aug",
									  "Sep",
									  "Oct",
									  "Nov",
									  "Dec" };

std::string LocalTime::GetTimeStr () const
{
	Msg timeStr;
	timeStr << wDay << "-" << _month [wMonth - 1] << "-" << wYear;
	std::string time (timeStr.c_str ());
	return time;
}

void Outbox::Submit (OutgoingMessage const & msg, RecipientList const & recipients, bool verbose)
{
	MapiFileDesc attachment;
	MapiMessage mailMsg;
	LocalTime localTime;
	Msg subjectLine;

	subjectLine << msg.GetSubject () << " - " << localTime.GetTimeStr ();
	mailMsg.ulReserved = 0;
	mailMsg.lpszNoteText = const_cast<char *>(msg.GetText ());
	mailMsg.lpszMessageType = 0;
	mailMsg.lpszDateReceived = 0;
	mailMsg.lpszConversationID = 0;
	mailMsg.flFlags = 0;
	mailMsg.lpOriginator = 0;
	mailMsg.nRecipCount = recipients.GetCount ();
	mailMsg.lpRecips = const_cast<MapiRecipDesc *>(recipients.GetMapiAddrList ());
	mailMsg.nFileCount = 0;
	mailMsg.lpFiles = 0;

	if (msg.HasAttachment ())
	{
		attachment.ulReserved = 0;
		attachment.flFlags = 0;
		attachment.nPosition = -1;
		attachment.lpszPathName = const_cast<char *>(msg.GetAttachFileName ());
		attachment.lpszFileName = const_cast<char *>(msg.GetAttachFileName ());
		attachment.lpFileType = 0;
		// Add message attachment
		mailMsg.nFileCount = 1;
		mailMsg.lpFiles = &attachment;
		// Modify message subject to display attchment file name
		subjectLine << " (" << msg.GetAttachFileName () << ")";
		// Check if attachment file exists
		if (!File::Exists (attachment.lpszPathName))
			throw Win::Exception ("Cannot find attachment", attachment.lpszPathName); 
	}

	mailMsg.lpszSubject = const_cast<char *>(subjectLine.c_str ());

	typedef ULONG (FAR PASCAL *Send) (LHANDLE session,
									  ULONG ulUIParam,
									  lpMapiMessage message,
									  FLAGS flFlags,
									  ULONG ulReserved);
	ULONG rCode;
	// Send message
	Send send = reinterpret_cast<Send>(_session.GetFunction ("MAPISendMail"));
	ULONG flags = verbose ? MAPI_DIALOG : 0; // If verbose request Simple MAPI send dialog for message late changes 
	rCode = send (_session.GetHandle (),
				  0,
				  &mailMsg,
				  flags,	
				  0);
	if (rCode != SUCCESS_SUCCESS)
		throw MapiException ("Cannot send message", rCode);
}
