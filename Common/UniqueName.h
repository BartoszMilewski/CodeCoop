#if !defined (UNIQUENAME_H)
#define UNIQUENAME_H
//----------------------------------
// (c) Reliable Software 1998 - 2006
//----------------------------------

#include "SerString.h"
#include "GlobalId.h"

// Revisit: split this class into UName and UniqueName: public UName, public Serializable
// and put only UName in the common directory

class UniqueName: public Serializable
{
public:
	UniqueName ()
		: _gidParent (gidInvalid)
	{}
	UniqueName (GlobalId gidParent, char const * name)
		: _gidParent (gidParent)
	{
		_name.assign (name);
	}
	UniqueName (GlobalId gidParent, std::string const & name)
		: _gidParent (gidParent),
		  _name (name)
	{}
	UniqueName (UniqueName const & uname)
		: _gidParent (uname.GetParentId ()),
		  _name (uname.GetName ())
	{}
    UniqueName (Deserializer & in, int version)
    { 
        Deserialize (in, version); 
    }
	void Init (GlobalId gidParent, char const * name)
	{
		_gidParent = gidParent;
		_name.assign (name);
	}
	void Init (GlobalId gidParent, std::string const & name)
	{
		_gidParent = gidParent;
		_name = name;
	}
	void Init (UniqueName const & uname)
	{
		_gidParent = uname.GetParentId ();
		_name = uname.GetName ();
	}
	GlobalId	GetParentId () const { return _gidParent; }
	std::string const & GetName () const { return _name; }
	void SetParentId (GlobalId gid)
	{
		_gidParent = gid;
	}
	void SetName (char const * name)
	{
		_name.assign (name);
	}
	void Down (char const * name)
	{
		FilePath path (_name);
		_name.assign (path.GetFilePath (name));
	}
	bool IsValid () const 
	{
		// If _name is empty then _gidParent must be equal gidInvalid -- this is
		// project's root folder unique name, otherwise _gidParent cannot be
		// equal gidInvalid, because project names always have known parent.
		return _name.empty () ? _gidParent == gidInvalid : _gidParent != gidInvalid;
	}

	bool IsRootName () const
	{
		return _name.empty () && _gidParent == gidInvalid;
	}
	bool IsEqual (UniqueName const & uname) const;
	bool IsStrictlyEqual (UniqueName const & uname) const;
	bool IsNormalized () const;
	bool operator< (UniqueName const & uname) const;
    void Serialize (Serializer& out) const;
    void Deserialize (Deserializer& in, int version);

private:
	GlobalId	_gidParent;
	SerString	_name;
};

#endif
