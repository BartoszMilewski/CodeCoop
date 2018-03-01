//
// (c) Reliable Software 1998
//

#include "Message.h"

void OutgoingMessage::SetSubject (char const * subject)
{
	_subject.assign (subject);
}

void OutgoingMessage::SetText (char const * text)
{
	_text.assign (text);
}

void OutgoingMessage::AddFileAttachment (std::string const & path)
{
	_attachmentPath = path;
}
