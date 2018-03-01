// -----------------------------------
// (c) Reliable Software, 2002 -- 2006
// -----------------------------------

#include "precompiled.h"
#include "Transport.h"
#include <Mail/EmailAddress.h>

#include <StringOp.h>
#include <File/Path.h>
#include <Dbg/Assert.h>

std::ostream& operator<<(std::ostream& os, Topology t)
{
	if (t.IsStandalone ())
		os << "standalone";
	if (t.IsHub ())
		os << "hub";
	if (t.IsSatellite ())
		os << "satellite";
	if (t.IsPeer ())
		os << "e-mail peer";
	if (t.IsRemoteSatellite ())
		os << "off-site satellite";
	if (t.IsTemporaryHub ())
		os << "off-site hub";
	return os;
}

std::string Transport::_methodNames [] =
{
	"Unknown",
	"Network",
	"Email",
};

void Transport::Init (std::string const & route)
{
	_route.assign (route);
	_method = Unknown;
	bool hasSlash = std::find_if (route.begin (), route.end (), FilePath::IsSeparator) != route.end ();
	if (route.empty ())
	{
		_method = Unknown;
	}
	else if (!hasSlash && Email::IsValidAddress (route))
	{
		// has @ sign but no path separators
		_method = Email;
	}
	else if (FilePath::IsValid (route.c_str ()))
	{
		FilePathSeq pathSeq (route.c_str ());
		if (pathSeq.HasDrive () || pathSeq.IsUNC ())
			_method = Network;
	}
}

std::string Transport::GetDestinationName() const
{
	if (IsEmail())
		return "Email";
	if (IsNetwork())
	{
		FullPathSeq pathSeq(_route.c_str());
		if (pathSeq.IsUNC())
		{
			return pathSeq.GetServerName();
		}
	}
	return "Local";
}

bool Transport::operator== (Transport const & trans) const
{
	if (trans.GetMethod () != GetMethod ())
		return false;

	if (IsNetwork ())
		return FilePath::IsEqualDir (trans.GetRoute (), GetRoute ());
	else
		return IsNocaseEqual (trans.GetRoute (), GetRoute ());
}

bool Transport::operator < (Transport const & trans) const
{
	if (_method != trans._method)
		return _method < trans._method;
	if (IsNetwork ())
		return FilePath::IsDirLess (GetRoute (), trans.GetRoute ());
	else
		return IsNocaseLess (GetRoute (), trans.GetRoute ());
}


void Transport::Serialize (Serializer & out) const
{
	_route.Serialize (out);
	out.PutLong (_method);
}

void Transport::Deserialize (Deserializer & in, int version)
{
	_route.Deserialize (in, version);
	_method = static_cast<Method> (in.GetLong ());
}

void TransportArray::Init (std::vector<Transport> & transports, Transport::Method active)
{
	_transports.swap (transports);
	_active = active;
	Assert (IsDefined (_active));
}

void TransportArray::ResetActive (Transport const & transport)
{
	if (transport.IsUnknown ())
		return;

	std::vector<Transport>::iterator it = std::find_if (_transports.begin (),
													    _transports.end (),
													    IsEqualMethod (transport.GetMethod ()));
	if (it == _transports.end ())
	{
		_transports.push_back (transport);
	}
	else
	{
		*it = transport;
	}
	_active = transport.GetMethod ();
}

Transport const & TransportArray::Get (Transport::Method method) const
{
	static Transport empty;
	std::vector<Transport>::const_iterator tr = std::find_if (_transports.begin (),
															  _transports.end (),
															  IsEqualMethod (method));
	if (tr == _transports.end ())
		return empty;
	return *tr;
}

Transport & TransportArray::GetEdit (Transport::Method method)
{
	std::vector<Transport>::iterator tr = std::find_if (_transports.begin (),
															  _transports.end (),
															  IsEqualMethod (method));
	Assert (tr != _transports.end ());
	return *tr;
}

Transport const & TransportArray::GetActive () const 
{
	static Transport empty;
	if (_active == Transport::Unknown)
		return empty;
	else
	{
		return Get (_active);
	}
}

bool TransportArray::IsDefined (Transport::Method method) const 
{
	return method == Transport::Unknown ?
						true : 
						std::find_if (_transports.begin (),
									  _transports.end (),
									  IsEqualMethod (method)) != _transports.end (); 
}

std::ostream& operator<<(std::ostream& os, Transport const & t)
{
	os << ReplaceNullPadding<'.'> (t.GetRoute ()) << " (";
	switch (t.GetMethod ())
	{
	case Transport::Unknown:
		os << "Unknown";
		break;
	case Transport::Network:
		os << "Network";
		break;
	case Transport::Email:
		os << "Email";
		break;
	default:
		os << "Error";
		break;
	}
	os << ")";
	return os;
}
