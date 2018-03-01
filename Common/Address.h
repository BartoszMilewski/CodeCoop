#if !defined (ADDRESS_H)
#define ADDRESS_H
//----------------------------------
//  (c) Reliable Software, 2002-2006
//----------------------------------

#include "SerString.h"
#include "Params.h"

// Special user ids
char const * const DispatcherAtHubId = "HUB";
char const * const DispatcherAtSatId = "SAT";
char const * const DispatcherAtRelisoft = "RELISOFT";

class Address: public Serializable
{
public:
	Address () {}
	Address (std::string const & hubId, std::string const & projName, std::string const & userId)
		: _projName (projName), _hubId (hubId), _userId (userId)
	{}
	Address  (Deserializer & in, int version)
	{
		Deserialize (in, version);
	}
	void Set (std::string const & hubId, std::string projName, std::string const & userId)
	{
		_hubId.assign (hubId);
		_projName.assign (projName);
		_userId.assign (userId);
	}
	void Set (Address const & address)
	{
		_hubId.assign (address.GetHubId ());
		_projName.assign (address.GetProjectName ());
		_userId.assign (address.GetUserId ());
	}
	bool IsEqualAddress (Address const & address) const;
	std::string const & GetProjectName () const { return _projName; }
	std::string const & GetHubId () const { return _hubId; }
	std::string const & GetUserId () const { return _userId; }
	void SetHubId (std::string const & hubId)
	{
		_hubId = hubId;
	}
	void SetProjectName (std::string const & projName)
	{
		_projName = projName;
	}
	void SetUserId (std::string const & userId)
	{
		_userId = userId;
	}
	bool IsHubDispatcher () const 
	{
		return strcmp (GetUserId ().c_str (), DispatcherAtHubId) == 0;
	}
	bool IsSatDispatcher () const 
	{
		return strcmp (GetUserId ().c_str (), DispatcherAtSatId) == 0;
	}
	bool IsRelisoftDispatcher () const
	{
		return strcmp (GetUserId ().c_str (), DispatcherAtRelisoft) == 0;
	}
	bool IsDispatcher () const 
	{
		return IsHubDispatcher () || IsSatDispatcher () || IsRelisoftDispatcher ();
	}

	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);
	void Dump (std::ostream & out) const;
	bool operator < (Address const & add) const;
private:
	SerString	_projName;
	SerString	_hubId;
	SerString	_userId;
};

#endif
