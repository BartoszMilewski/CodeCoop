#if !defined (HEADERDETAILS_H)
#define HEADERDETAILS_H
// ---------------------------
// (c) Reliable Software, 2000
// ---------------------------

#include <File/Path.h>
#include <string>

class HeaderDetails
{
public:
	HeaderDetails ();

	bool IsDestDataValid ();
	void DisplayDestErrors ();

	enum Destination
	{
		PublicInbox,
		PrivateInbox,
		PrivateOutbox,
		UserDefined
	};

	bool _wasCanceled;

	bool _toBeForwarded;
	bool _isDefect;
	bool _hasAddendums;

	bool _isProjectUnknown;
	bool _isSenderEmailUnknown;
	bool _isSenderLocationUnknown;
	bool _isRecipEmailUnknown;
	bool _isRecipLocationUnknown;

	std::string _projectName;
	std::string _senderEmail;
	std::string _senderId;
	std::string _recipEmail;
	std::string _recipId;

	std::string _destProject;
	std::string _destUserId;
	Destination _destFolder;
	FilePath	_destPath;
	std::string _scriptFilename;
};

#endif
