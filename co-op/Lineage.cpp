//-----------------------------------------
// (c) Reliable Software 1997 -- 2004
//-----------------------------------------

#include "precompiled.h"
#include "Lineage.h"

namespace Unit
{
	bool VerifyType (Type type)
	{
		switch (type)
		{
		case Set:
		case Member:
		case File:
		case Milestone:
		case SubProject:
		case Ignore:
			return true;
			break;
		default:
			return false;
			break;
		}
	}

	void ScriptId::Serialize (Serializer & out) const
	{
		out.PutLong (_scriptId);
		out.PutLong (_unitType);
	}

	void ScriptId::Deserialize (Deserializer & in, int version)
	{
		_scriptId = in.GetLong ();
		_unitType = static_cast<Unit::Type>(in.GetLong ());
	}
}

void Lineage::InitReverse (GidList const & reverseLineage)
{
	_gidList.clear ();
	_gidList.reserve (reverseLineage.size ());
	for (GidList::const_reverse_iterator iter = reverseLineage.rbegin ();
		 iter != reverseLineage.rend ();
		 ++iter)
	{
		GlobalId gid = *iter;
		_gidList.push_back (gid);
	}
}

void Lineage::ReInit (Lineage const & lineage)
{
	_gidList = lineage._gidList;
}

void Lineage::Append (GidList::const_iterator begin, GidList::const_iterator end)
{
	GidList::const_iterator it = begin;
	while (it != end)
	{
		_gidList.push_back (*it);
		++it;
	}
}

Lineage::CmpResult Lineage::CompareWith (Lineage const & lin, int & idxDiff) const
{
    // They both must start with the same reference version
    Assert (Count () > 0 && lin.Count () > 0);
    Assert (_gidList [0] == lin [0]);
    int lenShort = std::min (Count (), lin.Count ());
	int i = 1;
    for (i = 1; i < lenShort; i++)
        if (_gidList [i] != lin [i])
            break;
    idxDiff = i;
    if (i == lenShort)
    {
        if (Count () > lin.Count ())
            return Longer;
        else if (Count () < lin.Count ())
            return Shorter;
        else
            return Equal;
    }
    // There was a difference at index i
    GlobalIdPack gid1 (_gidList [i]);
    GlobalIdPack gid2 (lin [i]);
    if (gid1.GetUserId () < gid2.GetUserId ())
        return Higher;
    else
        return Lower;
}

void Lineage::Serialize (Serializer& out) const
{
    out.PutLong (_gidList.size ());
    for (unsigned int i = 0; i != _gidList.size (); ++i)
	{
        out.PutLong (_gidList [i]);
	}
}

void Lineage::Deserialize (Deserializer& in, int version)
{
	unsigned int len = in.GetLong ();
    for (unsigned int i = 0; i != len; ++i)
        _gidList.push_back (in.GetLong ());
}

bool Lineage::Verify () const
{
	for (unsigned int i = 0; i < _gidList.size (); ++i)
	{
		GlobalId id = _gidList [i];
		if ((id == 0 || id == gidInvalid) && i != 0)
			return false;
	}
	return true;
}

void UnitLineage::Serialize (Serializer& out) const
{
	Lineage::Serialize (out);
	out.PutLong (_type);
	out.PutLong (_unitId);
}

void UnitLineage::Deserialize (Deserializer& in, int version)
{
	Lineage::Deserialize (in, version);
	_type = static_cast<Unit::Type>(in.GetLong ());
	_unitId = in.GetLong ();
}

bool UnitLineage::Verify () const
{
	if (!Unit::VerifyType (_type))
		return false;
	if (_type == Unit::Set && _unitId != gidInvalid || _type != Unit::Set && _unitId == gidInvalid)
		return false;
	return Lineage::Verify ();
}

std::ostream & operator<<(std::ostream & os, Lineage const & lineage)
{
	if (lineage.Count () != 0)
	{
		unsigned int idx = lineage.Count ();
		do
		{
			--idx;
			GlobalIdPack pack (lineage [idx]);
			if (pack == lineage.GetReferenceId ())
				os << pack.ToSquaredString ();
			else
				os << pack.ToString () << ", ";
		} while (idx != 0);
	}
	return os;
}

std::ostream & operator<<(std::ostream & os, UnitLineage const & lineage)
{
	if (lineage.GetUnitId () == gidInvalid)
		os << "unit id not specified";
	else
		os << "unit id = 0x" << std::hex << lineage.GetUnitId ();
	os << " --> " << reinterpret_cast<Lineage const &>(lineage);
	return os;
}

std::ostream & operator<<(std::ostream & os, Unit::Type type)
{
	switch (type)
	{
	case Unit::Set:
		os << "Unit::Set";
		break;
	case Unit::Member:
		os << "Unit::Member";
		break;
	case Unit::File:
		os << "Unit::File";
		break;
	case Unit::Milestone:
		os << "Unit::Milestone";
		break;
	case Unit::SubProject:
		os << "Unit::SubProject";
		break;
	default:
		os << "Unit::Unknown; ";
		break;
	}
	return os;
}

std::ostream & operator<<(std::ostream & os, Unit::ScriptId id)
{
	GlobalIdPack idPack (id.Gid ());
	os << idPack << "    " << id.Type ();
	return os;
}