//
// (c) Reliable Software 1998-2003
//
#include "precompiled.h"
#include "EmailMessage.h"

#include <File/SafePaths.h>

void OutgoingMessage::SetSubject (std::string const & subject)
{
	_subject = subject;
}

void OutgoingMessage::SetText (std::string const & text)
{
	_text = text;
}

void OutgoingMessage::AddFileAttachment (std::string const & path)
{
	_attachments.push_back (path);
}

void OutgoingMessage::AddFileAttachment (SafePaths const & attPaths)
{
    for (SafePaths::iterator it = attPaths.begin (); it != attPaths.end (); ++it)
    {
	    _attachments.push_back (*it);
    }
}
