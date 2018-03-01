#if !defined (RECIPIENT_H)
#define RECIPIENT_H
// ---------------------------------
// (c) Reliable Software, 1999-2002
// ---------------------------------
#include "UserIdPack.h"
#include "Address.h"

class ScriptTicket;
class MailTruck;

class Recipient: public Address
{
public:
	Recipient () : _isRemoved (false) {}
    Recipient (std::string const & hubId, std::string const & project, std::string const & userId)
        : Address (hubId, project, userId), _isRemoved (false)
    {}
	Recipient (Address const & address)
		: Address (address), _isRemoved (false)
	{}
    virtual bool AcceptScript (ScriptTicket & script, MailTruck & truck, int recipIdx)
    {
        Assert (!"Method of abstract class called");
		return false;
    }

	bool IsRemoved () const { return _isRemoved; }

	bool RemovesMember (ScriptTicket & script, int recipIdx);

	void MarkRemoved (bool isRemoved) { _isRemoved = isRemoved; }

	bool HasEqualHubId (std::string const & hubId) const
	{
		return IsNocaseEqual (GetHubId (), hubId);
	}
	bool HasEqualProjectName (std::string const & projName) const
	{
		return IsNocaseEqual (GetProjectName (), projName);
	}
	bool HasEqualUserId (std::string const & userId) const
	{
		return IsNocaseEqual (GetUserId (), userId);
	}
	bool HasRandomUserId () const
	{
		UserIdPack userId (GetUserId ());
		return userId.IsRandom ();
	}
	void ChangeAddress (Address const & newAddress)
	{
		Set (newAddress);
	}

	// predicates
	class HasEqualAddress : public std::unary_function<Recipient const &, bool>
	{
	public:
		HasEqualAddress (Address const & address)
			: _address (address)
		{}

		bool operator () (Recipient const & recip) const
		{
			return recip.IsEqualAddress (_address);
		}
	private:
		Address const & _address;
	};

	class HasEqualProjectUser : public std::unary_function<Recipient const &, bool>
	{
	public:
		HasEqualProjectUser (Address const & address)
			: _project (address.GetProjectName ()),
			  _uid (address.GetUserId ())
		{}

		bool operator () (Recipient const & recip) const
		{
			return recip.HasEqualProjectName (_project)
				&& recip.HasEqualUserId (_uid);
		}
	private:
		std::string const & _project;
		std::string const & _uid;
	};
private:
	bool			_isRemoved;
};

#endif
