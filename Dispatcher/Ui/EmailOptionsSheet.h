#if !defined (EMAILOPTIONSSHEET_H)
#define EMAILOPTIONSSHEET_H
// ----------------------------------
// (c) Reliable Software, 2005 - 2008
// ----------------------------------

namespace Win { class MessagePrepro; }

namespace Email
{
	class Manager;

	bool RunOptionsSheet (std::string const & myEmail,
						  Email::Manager & emailMan,
						  Win::Dow::Handle win,
						  Win::MessagePrepro * msgPrepro);
}

#endif
