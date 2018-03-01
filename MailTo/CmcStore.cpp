//
// (c) Reliable Software 1998
//

#include "CmcStore.h"
#include "Message.h"
#include "CmcRecipientList.h"
#include "CmcSession.h"
#include "CmcEx.h"

void Outbox::Submit (OutgoingMessage const & msg, RecipientList const & recipients, bool verbose)
{
	CMC_time timeNow;
	CMC_attachment attachment;
	CMC_message mailMsg;

	mailMsg.message_reference = 0;
	mailMsg.message_type = "CMC: IPM";
	mailMsg.subject = const_cast<char *>(msg.GetSubject ());
	mailMsg.time_sent = timeNow;
	mailMsg.text_note = const_cast<char *>(msg.GetText ());
	mailMsg.recipients = const_cast<CMC_recipient *>(recipients.GetCmcAddrList ());
	mailMsg.attachments = 0;
	mailMsg.message_flags = CMC_MSG_LAST_ELEMENT;
	mailMsg.message_extensions = 0;

	if (msg.HasAttachment ())
	{
		attachment.attach_title = const_cast<char *>(msg.GetAttachFileName ());
		attachment.attach_type = CMC_ATT_OID_BINARY;
		attachment.attach_filename = const_cast<char *>(msg.GetAttachFileName ());
		attachment.attach_flags = CMC_ATT_LAST_ELEMENT;
		attachment.attach_extensions = 0;
		// Add message attachment
		mailMsg.attachments = &attachment;
	}

	typedef CMC_return_code (*Send) (CMC_session_id session,
									 CMC_message const * message,
									 CMC_flags flags,
									 CMC_ui_id ui_id,
									 CMC_extension * send_extensions);
	CMC_return_code rCode;
	// Send message
	Send cmcSend = reinterpret_cast<Send>(_session.GetCmcFunction ("cmc_send"));
	rCode = cmcSend (_session.GetId (),
					 &mailMsg,
					 CMC_LOGON_UI_ALLOWED,
					 0,
					 0);
	if (rCode != CMC_SUCCESS)
		throw CmcException ("Cannot send message", rCode);
}
