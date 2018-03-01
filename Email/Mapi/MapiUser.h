#if !defined (MAPIUSER_H)
#define MAPIUSER_H
//-------------------------------------
// (c) Reliable Software 2001
// ------------------------------------

#include "MapiIface.h"

namespace Mapi
{
	class DefaultDirectory;

	class MailUser
	{
	public:
		MailUser () {}
		operator Com::UnknownPtr & () { return _user; }

	protected:
		Interface<IMailUser>	_user;
	};

	class CurrentUser : public MailUser
	{
	public:
		CurrentUser (DefaultDirectory & defDir);

		bool IsValid () const { return _isValid; }
		void GetIdentity (std::string & name, std::string & emailAddr);

	private:
		bool	_isValid;
	};
}

#endif
