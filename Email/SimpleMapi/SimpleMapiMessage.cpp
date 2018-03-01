// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------
#include "precompiled.h"
#include "SimpleMapiMessage.h"
#include "EmailMessage.h"
#include <File/File.h>

SimpleMapi::Message::Message (
	OutgoingMessage const & blueprint, 
	MapiRecipDesc const * recipients, 
	unsigned int recipientCount)
{
	std::fill_n (reinterpret_cast<unsigned char *>(&_msg), sizeof (MapiMessage), 0);

	_msg.lpszSubject = const_cast<char *>(blueprint.GetSubject ().c_str ());
	_msg.lpszNoteText = const_cast<char *>(blueprint.GetText ().c_str ());
	_msg.nRecipCount = recipientCount;
	_msg.lpRecips = const_cast<MapiRecipDesc *>(recipients);
	_msg.nFileCount = 0;
	_msg.lpFiles = 0;

	// Add message attachments
	int count = blueprint.GetAttachmentCount ();
	if (count > 0)
	{
		_attachments.reserve (count);
		for (OutgoingMessage::AttPathIter it = blueprint.AttPathBegin (); 
			 it != blueprint.AttPathEnd (); 
			 ++it)
		{
			char const * attPath = it->c_str ();
			// Check if attachment file exists
			if (!File::Exists (attPath))
				throw Win::Exception ("Cannot find message attachment", attPath); 

			MapiFileDesc att;
			att.ulReserved = 0;
			att.flFlags = 0;
			att.nPosition = -1;
			att.lpszPathName = const_cast<char *>(attPath);
			att.lpszFileName = 0;
			att.lpFileType = 0;

			_attachments.push_back (att);
		}
		Assert (count == _attachments.size ());
		_msg.lpFiles    = &_attachments [0];
		_msg.nFileCount = _attachments.size ();
	}
}
