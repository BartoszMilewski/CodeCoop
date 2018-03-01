#if !defined (MEMBERDESCRIPTION_H)
#define MEMBERDESCRIPTION_H
//------------------------------------
//  (c) Reliable Software, 2002 - 2007
//------------------------------------

#include "Serialize.h"
#include "SerString.h"
#include "GlobalId.h"

// Project member description stored in the log

class MemberDescription : public Serializable
{
public:
    MemberDescription () {}
    MemberDescription (std::string const & name,
					   std::string const & hubId,
					   std::string const & comment, 
					   std::string const & license, 
					   std::string const & userId)
        : _name (name),
          _hubId (hubId),
          _comment (comment),
          _license (license),
		  _userId (userId)
    {}
    MemberDescription (MemberDescription const & description)
        : _name (description.GetName ()),
          _hubId (description.GetHubId ()),
          _comment (description.GetComment ()),
          _license (description.GetLicense ()),
		  _userId (description.GetUserId ())
    {}

    MemberDescription (MemberDescription && description)
        : _name (std::move(description._name)),
          _hubId (std::move(description._hubId)),
          _comment (std::move(description._comment)),
          _license (std::move(description._license)),
		  _userId (std::move(description._userId))
    {}

    MemberDescription (Deserializer &in, int version)
    {
        Deserialize (in, version);
    }

	void Pad (); // make all fields fixed size

	void SetName (std::string const & name) { _name = name; }
	void SetHubId (std::string const & hubId) { _hubId = hubId; }
	void SetComment (std::string const & comment) { _comment = comment; }
	void SetLicense (std::string const & license) { _license = license; }
	void SetUserId (std::string const & userId) { _userId = userId; }
	void Clear ();

	bool IsEqual (MemberDescription const & desc) const;

    std::string const & GetName ()  const { return _name; }
    std::string const & GetHubId () const { return _hubId; }
    std::string const & GetComment () const { return _comment; }
    std::string const & GetLicense () const { return _license; }
	std::string const & GetUserId () const { return _userId; }

	bool IsLicensed (int & version, int & seats, char & product) const;
	bool IsLicensed () const;
	void VerifyLicense () const;

    // Serializable interface

    void Serialize (Serializer& out) const;
    void Deserialize (Deserializer& in, int version);

private:
    SerString _name;
    SerString _hubId;
    SerString _comment;
    SerString _license;
	SerString _userId;
};

class CurrentMemberDescription : public MemberDescription
{
public:
	CurrentMemberDescription ();
};

class MemberNameTag : public std::string
{
public:
	MemberNameTag (std::string const & name, UserId userId);
	MemberNameTag (std::string const & name, std::string const & userId);
};

class MembershipUpdateComment : public std::string
{
public:
	MembershipUpdateComment (MemberNameTag const & tag, std::string const & comment);
};

#endif
