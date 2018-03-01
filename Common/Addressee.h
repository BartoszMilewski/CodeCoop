#if !defined (ADDRESSEE_H)
#define ADDRESSEE_H
//----------------------------------
// (c) Reliable Software 1998 - 2005
// ---------------------------------

#include "Serialize.h"
#include "SerString.h"
#include "SerVector.h"

class Addressee : public Serializable
{
public:
    Addressee (Deserializer& in, int version)
		: _receivedScript (false)
	{
		Deserialize (in, version);
	}
	Addressee (Addressee const & addressee);
	Addressee (std::string const & hubId, std::string const & userId = std::string ())
       : _hubId (hubId),
	     _userId (userId),
		 _receivedScript (false)
    {}

	std::string const & GetHubId () const { return _hubId; }
	// User id decoding
	std::string const & GetStringUserId () const { return _userId; };
    std::string const & GetDisplayUserId () const;
	bool IsUserIdEmpty () const { return _userId.empty (); }
	bool HasWildcardUserId () const;
	bool IsHubDispatcher () const; 
	bool IsSatDispatcher () const; 
	bool IsDispatcher () const 
	{
		return IsHubDispatcher () || IsSatDispatcher ();
	}
	// Comparisons
	bool IsEqual (std::string const & hubId, std::string const & userId) const;
	bool IsEqual (Addressee const & addr) const
	{
		return IsEqual (addr._hubId, addr._userId);
	}
    bool ReceivedScript () const { return _receivedScript; }
    void SetDeliveryFlag (bool delivered) { _receivedScript = delivered; }

    void Serialize (Serializer& out) const;
    void Deserialize (Deserializer& in, int version);

private:
    SerString _hubId;
    SerString _userId;
    bool      _receivedScript;
};

std::ostream& operator<<(std::ostream& os, Addressee const & addressee);

class AddresseeList : public SerVector<Addressee>
{
public:
	typedef SerVector<Addressee>::iterator Iterator;
	typedef SerVector<Addressee>::const_iterator ConstIterator;
};

std::ostream& operator<<(std::ostream& os, AddresseeList const & addrList);

#endif
