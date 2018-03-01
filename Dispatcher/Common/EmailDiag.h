#if !defined (EMAILDIAG_H)
#define EMAILDIAG_H
//------------------------------------
//  (c) Reliable Software, 1999 - 2008
//------------------------------------

#include "EmailConfig.h"

class DiagFeedback;
class DiagnosticsProgress;

namespace Email
{
	class Manager;

	// Performs the test of the email client
	class Diagnostics
	{
	public:
		Diagnostics (std::string const & myEmail,
					 DiagFeedback & feedback,
					 DiagnosticsProgress & progress);

		Email::Status Run (Email::Manager & emailMan);

	private:
		static unsigned const InboxMsgDisplayCount = 7;
		static unsigned const MaxInboxMsgDisplayCount = 50;

		Email::Status TestOutgoing (Email::Manager & emailMan);
		Email::Status TestIncoming (Email::Manager & emailMan);
		void ExamineEmailClient ();
		Email::Status CheckIdentities ();
		Email::Status ListInbox (Email::Manager & emailMan);

		void DisplayTechnologyInfo (std::string const & technology);
		void DisplayServerSettings (std::string const & server, short port, int timeout, bool useSSL);
		bool CheckForCancel ();

	private:
		std::string				_myEmail;
		Email::Status			_emailClientStatus;
		DiagFeedback		&	_feedback;
		DiagnosticsProgress &	_progress;
	};
}

#endif
