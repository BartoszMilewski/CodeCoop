#if !defined (MAPIMESSAGE_H)
#define MAPIMESSAGE_H
//
// (c) Reliable Software 1998
//

#include "LightString.h"

class RecipientList;
class MapiAddrList;

class OutgoingMessage
{
public:
	void SetSubject (char const * subject);
	void SetText (char const * text);
	void AddFileAttachment (std::string const & path);

	char const * GetSubject () const { return _subject.c_str (); }
	char const * GetText () const { return _text.c_str (); }
	char const * GetAttachFileName () const { return _attachmentPath.c_str (); }
	bool HasAttachment () const { return _attachmentPath.length () != 0; }

private:
	std::string	_subject;
	std::string	_text;
	std::string	_attachmentPath;
};

#endif
