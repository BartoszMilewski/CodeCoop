#if !defined (INVITEE_H)
#define INVITEE_H
// ---------------------------
// (c) Reliable Software, 2006
// ---------------------------

#include "Address.h"

class Invitee : public Address
{
public:
	Invitee () {}
	Invitee (Address const & address, 
			std::string const & userName, 
			std::string const & computerName,
			bool isObserver)
		: Address (address),
		  _user (userName),
		  _computer (computerName),
		  _isObserver (isObserver)
	{}
	std::string const & GetUserName () const { return _user; }
	std::string const & GetComputerName () const { return _computer; }
	bool IsObserver () const { return _isObserver; }

	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);
private:
	SerString	_user;
	SerString	_computer;
	bool		_isObserver;
};

#endif
