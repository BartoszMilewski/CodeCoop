#if !defined (SESSION_H)
#define SESSION_H
//
// (c) Reliable Software 1998 -- 2004
//
#include "EmailTransport.h"
#include <Sys/Dll.h>

#include <mapi.h>

class Dll;

namespace SimpleMapi
{
	class Session: public Email::SessionInterface
	{
	public:
		Session ();
		~Session ();

		LHANDLE GetHandle () const { return _session; }
		template <class T>
		void GetFunction (std::string const & funcName, T & funPointer)
		{
			return _simpleMapiDll->GetFunction (funcName, funPointer); 
		}

	private:
		Session (Session const &);
		Session & operator= (Session const &);
		std::unique_ptr<Dll>	_simpleMapiDll;
		LHANDLE				_session;
	};
}

#endif
