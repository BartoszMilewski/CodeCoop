#if !defined (TRANSPORT_H)
#define TRANSPORT_H
// -----------------------------------
// (c) Reliable Software, 2002 -- 2005
// -----------------------------------

#include "SerString.h"
#include <File/Path.h>

#include <iosfwd>
#include <bitset>

//------------------
//					HasHub?	HasSat? UseEmail?
// Standalone		0		0		0		 
// Satellite		1		0		0		 
// Off-site Sat		1		0		1		 
// Hub				0		1		*		 
// Peer				0		0		1		 
// Off-site Hub		1		1		1		 
//------------------
class Topology
{
	enum { bitHasHub = 0,
		   bitHasSat,
		   bitUseEmail,
		   bitUseNet,
		   BitCount };
public:
	Topology (unsigned long long bits = 0) : _bits (bits) {}
	unsigned long ToUlong () const
	{
		Assert (BitCount < std::numeric_limits<unsigned long>::digits);
		return _bits.to_ulong ();
	}
	bool operator == (Topology const & topology) const
	{
		return ToUlong () == topology.ToUlong ();
	}
	bool operator != (Topology const & topology) const
	{
		return ! operator== (topology);
	}
	void SetHasHub (bool value) { _bits.set (bitHasHub, value); }
	void SetHasSat (bool value) { _bits.set (bitHasSat, value); }
	void SetUseEmail (bool value) { _bits.set (bitUseEmail, value); }
	void SetUseNet (bool value) { _bits.set (bitUseNet, value); }
	bool HasHub () const { return _bits.test (bitHasHub); }
	bool HasSat () const { return _bits.test (bitHasSat); }
	bool UsesEmail () const { return _bits.test (bitUseEmail); }
	bool UsesNet () const { return _bits.test (bitUseNet); }

	bool IsStandalone () const { return _bits.to_ulong () == 0; }
	bool IsHub () const { return _bits.test (bitHasSat) && !_bits.test (bitHasHub); }
	bool IsPeer () const { return !_bits.test (bitHasSat) &&
								  !_bits.test (bitHasHub) &&
								  UsesEmail (); }
	bool IsHubOrPeer () const { return IsHub () || IsPeer (); }
	bool IsTemporaryHub () const { return _bits.test (bitHasHub) && _bits.test (bitHasSat); }
	bool IsSatellite () const { return _bits.test (bitHasHub) && 
									   !_bits.test (bitHasSat) &&
									   !_bits.test (bitUseEmail); }
	bool IsRemoteSatellite () const { return _bits.test (bitHasHub) && 
											 !_bits.test (bitHasSat) &&
											 _bits.test (bitUseEmail); }

	// reset the whole configuration
	void MakeStandalone ()		{ _bits.reset (); }
	void MakeSatellite ()		{ _bits.reset (); _bits.set (bitHasHub); }
	void MakeRemoteSatellite () { _bits.reset (); _bits.set (bitHasHub); _bits.set (bitUseEmail); }
	void MakeHub ()				{ _bits.reset (); _bits.set (bitHasSat);  SetUseNet (true); }
	void MakeTemporaryHub ()	{ _bits.reset (); _bits.set (bitHasHub); _bits.set (bitHasSat); 
								  _bits.set (bitUseEmail); }
	void MakePeer ()			{ _bits.reset (); _bits.set (bitUseEmail); }

private:
	std::bitset<BitCount>	_bits;
};

std::ostream& operator<<(std::ostream& os, Topology t);

class Transport : public Serializable
{
public:
	enum Method
	{
		Unknown = 0,
		Network = 1,
		Email = 2,
		MethodCount = 3
	};
	
public:
	Transport (): _method (Unknown) {}
	explicit Transport (std::string const & route) { Init (route); }
	Transport (std::string const & route, Transport::Method method)
		: _route (route), 
		  _method (method)
	{}
	Transport (FilePath const & fwdPath)
		: _route (fwdPath.ToString ()),
		  _method (Network)
	{}
	Transport (Deserializer & in, int version)
		: _method (Unknown)
	{
		Deserialize (in, version);
	}
	// Guess transport kind from string
	void Init (std::string const & route);
	Transport::Method GetMethod () const { return _method; }
	std::string const & GetRoute () const { return _route; }
	std::string GetDestinationName() const;
	void Set (std::string const & route, Transport::Method method)
	{
		_route.assign (route);
		_method = method;
	}
	void Set (std::string route) { Init (route); }
	bool IsUnknown () const { return _method == Unknown; }
	bool IsNetwork () const { return _method == Network; }
	bool IsEmail () const { return _method == Email; }

	bool operator== (Transport const & trans) const;
	bool operator!= (Transport const & trans) const
	{
		return !operator== (trans);
	}
	bool operator < (Transport const & trans) const;
	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);

	// for UI usage
	std::string const & GetMethodName () const { return _methodNames [_method]; }
	static std::string const & GetMethodName (unsigned int method)
	{
		Assume (method < Transport::MethodCount, "Invalid transport method");
		return _methodNames [method];
	}
private:
	static std::string _methodNames [];
	SerString	_route;
	Method		_method;
};

std::ostream& operator<<(std::ostream& os, Transport const & t);
	
class TransportArray
{
public:
	TransportArray () : _active (Transport::Unknown) {}
	void Init (std::vector<Transport> & transports, Transport::Method active);
	std::vector<Transport> const & Get () const { return _transports; }
	Transport const & GetActive () const;
	Transport const & Get (Transport::Method method) const;
	Transport & GetEdit (Transport::Method method);
	void ResetActive (Transport const & transport);
	void Add (Transport const & transport) { _transports.push_back (transport); }
	Transport::Method GetActiveMethod () const { return _active; }
	void SetActiveMethod (Transport::Method method) { _active = method; }
	bool IsDefined (Transport::Method method) const;
private:
	class IsEqualMethod
	{
	public:
		IsEqualMethod (Transport::Method method)
			: _method (method)
		{}
		bool operator () (Transport const & transport) const 
		{ 
			return transport.GetMethod () == _method; 
		}
	private:
		Transport::Method _method;
	};
private:
	std::vector<Transport>	_transports;
	Transport::Method		_active;
};

#endif
