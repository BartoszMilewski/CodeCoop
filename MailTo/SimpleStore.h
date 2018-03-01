#if !defined (SIMPLESTORE_H)
#define SIMPLESTORE_H
//
// (c) Reliable Software 1998
//

class Session;
class OutgoingMessage;
class RecipientList;

class Outbox
{
public:
	Outbox (Session & session)
		: _session (session)
	{}

	void Submit (OutgoingMessage const & msg, RecipientList const & recipients, bool verbose);

private:
	Session & _session;
};

#endif
