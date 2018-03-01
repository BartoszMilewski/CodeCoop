#if !defined (DEFAULTEMAILADDRESS_H)
#define DEFAULTEMAILADDRESS_H
//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

namespace Email
{
	class Manager;
	std::string GetDefaultAddress (Email::Manager & emailMan);
}

#endif
