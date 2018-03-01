//------------------------------------
//  (c) Reliable Software, 2002 - 2007
//------------------------------------

#include "precompiled.h"
#include "MemberDescription.h"
#include "UserIdPack.h"
#include "Crypt.h"
#include "OutputSink.h"
#include "Registry.h"
#include "Global.h"

void MemberDescription::Clear ()
{
    _name.clear ();
    _hubId.clear ();
    _comment.clear ();
    _license.clear ();
	_userId.clear ();
}

bool MemberDescription::IsEqual (MemberDescription const & desc) const
{
	return IsNocaseEqual (_name, desc.GetName ())		&&
		   IsNocaseEqual (_hubId,desc.GetHubId ())		&&
		   IsNocaseEqual (_comment, desc.GetComment ())	&&
		   IsNocaseEqual (_license, desc.GetLicense ()) &&
		   IsNocaseEqual (_userId, desc.GetUserId ());
}

void MemberDescription::Pad ()
{
    _name.Pad (32);
    _hubId.Pad (64);
    _comment.Pad (64);
    _license.Pad (128);
	_userId.Pad (32);
}

void MemberDescription::Serialize (Serializer& out) const
{
    _name.Serialize (out);
    _hubId.Serialize (out);
    _comment.Serialize (out);
    _license.Serialize (out);
	_userId.Serialize (out);
}

void MemberDescription::Deserialize (Deserializer& in, int version)
{
    _name.Deserialize (in, version);
    _hubId.Deserialize (in, version);
    _comment.Deserialize (in, version);
    _license.Deserialize (in, version);
	if (version > 24)
		_userId.Deserialize (in, version);
}

bool MemberDescription::IsLicensed (int & ver, int & seats, char & product) const
{
	if (!_license.empty ())
		return DecodeLicense (_license, ver, seats, product);

	return false;
}

bool MemberDescription::IsLicensed () const
{
	int ver, seats;
	char product;
	if (!_license.empty ())
		return DecodeLicense (_license, ver, seats, product);

	return false;
}

void MemberDescription::VerifyLicense () const
{
	std::string const & name = GetName ();
	if (name == UnknownName && _license.empty ())
		return;	// Unknown project member name and empty license -- don't verify license

	int ver = 0;
	int seats = 0;
	char product = '\0';
	if (IsLicensed (ver, seats, product))
		return;
#if defined BETA
	return;
#endif;
	// Warn about lack of valid license
	MemberNameTag nameTag (_name, _userId);
	std::string info (nameTag);
	info += ", hub's email address: ";
	info += _hubId;
	info += "\n";
	if (seats == 0)
	{
		info += "uses Code Co-op without a valid license.";
	}
	else if (ver < 4) // Concerning old versions before 4.0
	{
		info += "is not licensed to use the current version of Code Co-op.\n";
		info += "This user's license is only valid for version ";
		info += ToString (ver);
		info += " of Code Co-op.";
	}
	else
		return;  // don't nag for licensed users of version 3.x

	TheOutput.Display (info.c_str ());
}

// Project member description stored in the registry

CurrentMemberDescription::CurrentMemberDescription ()
{
	Registry::ReadUserDescription (*this);
}

MemberNameTag::MemberNameTag (std::string const & name, UserId userId)
: std::string (name.c_str ())
{
    std::ostringstream out;
	out << " (id: " << std::hex << userId << ')';
	*this += out.str ();
}

MemberNameTag::MemberNameTag (std::string const & name, std::string const & userId)
: std::string (name.c_str ())
{
	UserIdPack idPack (userId.c_str ());
	*this += " (id: ";
	*this += idPack.GetUserIdString ();
	*this += ')';
}

MembershipUpdateComment::MembershipUpdateComment (MemberNameTag const & tag, std::string const & comment)
	: std::string ("User ")
{
	*this += tag;
	*this += ' ';
	*this += comment;
}

#if !defined (NDEBUG) && !defined (BETA)

// Unit Test
void MemberDescriptionTest ()
{
	CurrentMemberDescription descPad;
	descPad.Pad ();
	MemberNameTag tagPad (descPad.GetName (), descPad.GetUserId ());

	CurrentMemberDescription desc;
	MemberNameTag tag (desc.GetName (), desc.GetUserId ());
	std::string strPad (tagPad);
	std::string str (tag);
	Assert (strPad == str);
}

#endif