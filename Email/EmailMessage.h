#if !defined (EMAILMESSAGE_H)
#define EMAILMESSAGE_H
//----------------------------------
// (c) Reliable Software 1998 - 2005
//----------------------------------

class SafePaths;

class OutgoingMessage
{
public:
	OutgoingMessage ()
		: _useBccRecipients (false)
	{}
	
	typedef std::vector<std::string>::const_iterator AttPathIter;

	void SetSubject (std::string const & subject);
	void SetText (std::string const & text);
	void SetBccRecipients (bool flag) { _useBccRecipients = flag; }
	void AddFileAttachment (std::string const & path);
	void AddFileAttachment (SafePaths const & attPaths);

	std::string const & GetSubject () const { return _subject; }
	std::string const & GetText () const { return _text; }
	bool UseBccRecipients () const { return _useBccRecipients; }
	int GetAttachmentCount () const { return _attachments.size (); }
	AttPathIter AttPathBegin () const { return _attachments.begin (); }
	AttPathIter AttPathEnd () const { return _attachments.end (); }
private:
	std::string					_subject;
	std::string					_text;
	std::vector<std::string>	_attachments;
	bool						_useBccRecipients;
};

#endif
