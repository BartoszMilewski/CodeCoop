#if !defined (POP3INBOXITERATOR_H)
#define POP3INBOXITERATOR_H
//---------------------------
// (c) Reliable Software 2005
// --------------------------
#include "EmailTransport.h"
#include <Mail/Pop3.h>

class Pop3InboxIterator : public InboxIteratorInterface
{
public:
	Pop3InboxIterator (Pop3::Session & session, bool isDeleteNonCoopMsg);

	// InboxIteratorInterface
	void Advance ();
	bool AtEnd () const { return _retriever.AtEnd (); }
	void RetrieveAttachements (SafePaths & attPaths);
	std::string const & GetSubject () const 
	{
		Assert (!AtEnd ());
		return _subject;
	}
	bool IsDeleteNonCoopMsg () const { return _isDelNonCoopMsg; }
	bool DeleteMessage () throw ();
	void CleanupMessage () throw ();

private:
	void RetrieveSubject ();
private:
	bool			 const	_isDelNonCoopMsg;
	bool					_isLogging;

	Pop3::MessageRetriever	_retriever;
	std::string				_subject;
};

#endif
