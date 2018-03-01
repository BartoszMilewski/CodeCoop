#if !defined (BADEMAILEXPT_H)
#define BADEMAILEXPT_H
// ---------------------------
// (c) Reliable Software, 2006
// ---------------------------

#include <Ex/WinEx.h>

namespace Email
{
	class BadEmailException : public Win::InternalException
	{
	public:
		BadEmailException (char const * msg, char const * moreInfo, std::string const & emailAddress)
			: Win::InternalException (msg, moreInfo),
			  _address (emailAddress)
		{}
		std::string const & GetAddress () const { return _address; }
	private:
		std::string	const _address;
	};
}

#endif
