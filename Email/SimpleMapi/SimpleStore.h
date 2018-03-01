#if !defined (STORE_H)
#define STORE_H
// ---------------------------------
// (c) Reliable Software 1998 - 2005
// ---------------------------------

class OutgoingMessage;

namespace SimpleMapi
{
	class Session;
	class RecipientList;

	class Outbox
	{
	public:
		Outbox (Session & session)
			: _session (session)
		{}

		void Submit (OutgoingMessage const & msg, RecipientList const & recipients, bool verbose);
		void Save (OutgoingMessage const & msg, RecipientList const & recipients);

	private:
		Session & _session;
	};
}

#endif
