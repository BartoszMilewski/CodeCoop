// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------
#include <WinLibBase.h>
#include "Pop3Message.h"
#include "Pop3.h"
#include "Base64.h"
#include <File/Path.h>
#include <File/FileIo.h>
#include <File/SafePaths.h>

void Pop3::Message::SaveAttachments (FilePath const & destFolder, SafePaths & attPaths) const
{
	for (auto_vector<Pop3::Attachment>::const_iterator att = _attachments.begin ();
		 att != _attachments.end ();
		 ++att)
	{
		// Revisit: handle the case when a file with the specified name already exists
		std::string destPath (destFolder.GetFilePath ((*att)->GetName ()));
		FileIo outFile (destPath, File::OpenAlwaysMode ());
		attPaths.Remember (destPath);

		(*att)->Save (outFile);
	}
}

void Pop3::Message::OnHeadersStart ()
{
}

void Pop3::Message::OnHeader (
		std::string const & name,
		std::string const & value,
		std::string const & attribute)
{
	// we are interested only in message subject -- 
	// we assume that it is the first "Subject" passed
	if (_subject.empty () && IsNocaseEqual (name, "Subject"))
		_subject = value;
}

void Pop3::Message::OnHeadersEnd ()
{
}

void Pop3::Message::OnBodyStart ()
{
}

void Pop3::Message::OnBodyEnd ()
{
}

void Pop3::Message::OnAttachment (GenericInput<'\0'> & input, std::string const & filename)
{
	Base64::Decoder decoder;
	std::unique_ptr<Attachment> attachment (new Pop3::Attachment (filename));
	Pop3::Attachment::Storage::Output output (attachment->GetStorage ());
	try
	{
		decoder.Decode (input, output);
	}
	catch (Base64::Exception e)
	{
		throw Pop3::MsgCorruptException (e.GetMessage ());
	}
	_attachments.push_back (std::move(attachment));
}

void Pop3::Message::OnText (std::string const & text)
{
	// we are interested only in the main message text -- 
	// we assume that it is the first text passed
	if (_text.empty ())
		_text = text;
}
