//------------------------------------
//  (c) Reliable Software, 2003 - 2006
//------------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "ScriptHeader.h"

#include <TimeStamp.h>

#include <sstream>

void ScriptHeader::CopyChunkInfo (ScriptHeader const & hdr)
{
	_partNumber = hdr.GetPartNumber ();
	_partCount = hdr.GetPartCount ();
	_maxChunkSize = hdr.GetMaxChunkSize ();
}

// Any changes to data layout must be propagated
// to ScriptSubHeader class serialization !
void ScriptHeader::Serialize (Serializer& out) const
{
	out.PutLong (_partNumber);
	out.PutLong (_partCount);
	out.PutLong (_maxChunkSize);
    _comment.Serialize (out);
    _projectName.Serialize (out);
	out.PutLong (_scriptId);
    out.PutLong (_scriptKind.GetValue ());
	out.PutLong (_unitType);
	out.PutLong	(_unitId);
    out.PutLong (_timeStamp);
    _lineage.Serialize (out);
	unsigned int count = _sideLineages.size ();
	out.PutLong	 (count);
	for (unsigned int i = 0; i < count; ++i)
	{
		_sideLineages [i]->Serialize (out);
	}
}

void ScriptHeader::Deserialize (Deserializer& in, int version)
{
	_isFromVersion40 = (version < 41);
	if (version > 31)
	{
		if (version > 44)
		{
			_partNumber = in.GetLong ();
			_partCount = in.GetLong ();
			_maxChunkSize = in.GetLong ();
		}
		else
		{
			_partNumber = 1;
			_partCount = 1;
			_maxChunkSize = 0;
		}
		_comment.Deserialize (in, version);
		_projectName.Deserialize (in, version);
	}
	if (version > 38)
		_scriptId = in.GetLong ();

	if (version < 40)
	{
		unsigned long oldValue = in.GetLong ();
		ScriptKind newKind;
		newKind.InitFromOldScriptKind (oldValue);
		_scriptKind = newKind;
	}
	else
	{
		_scriptKind = ScriptKind (in.GetLong ());
	}

	if (version < 39)
	{
		_unitType = Unit::Set;
		_unitId = gidInvalid;
	}
	else
	{
		_unitType = static_cast<Unit::Type>(in.GetLong ());
		_unitId = in.GetLong ();
	}
	if (version < 13)
		_timeStamp = 0;
	else
        _timeStamp = in.GetLong ();

	_lineage.Deserialize (in, version);

	if (version < 39)
	{
		// Script id is the last gid in the lineage
		_scriptId = _lineage.GetLastScriptId ();
		_lineage.PopBack ();
	}
	else
	{
		unsigned int count = in.GetLong ();
		for (unsigned int i = 0; i < count; ++i)
		{
			std::unique_ptr<UnitLineage> tmp (new UnitLineage (in, version));
			_sideLineages.push_back (std::move(tmp));
		}
	}

	if (version < 32)
	{
		_comment.Deserialize (in, version);
		_projectName.Deserialize (in, version);
	}

	if (version >= 36 && version <= 44)
	{
		// used to be reserved
		in.GetLong ();
		in.GetLong ();
	}
}

void ScriptHeader::Verify () const throw (Win::Exception)
{
	std::ostringstream out;
	if (_projectName.empty ())
	{
		out << *this;
		throw Win::Exception ("Corrupted script header: missing project name.", out.str ().c_str ());
	}
	if (_maxChunkSize == 0 && _partNumber != _partCount)
	{
		out << *this;
		throw Win::Exception ("Corrupted script header: illegal chunk size.", out.str ().c_str ());
	}
	if (!Unit::VerifyType (_unitType))
	{
		out << *this;
		throw Win::Exception ("Corrupted script header: illegal unit type.", out.str ().c_str ());
	}
	if (!_scriptKind.Verify (_isFromVersion40))
	{
		out << *this;
		throw Win::Exception ("Corrupted script header: illegal script kind.", out.str ().c_str ());
	}
	if (_comment.empty ())
	{
		out << *this;
		throw Win::Exception ("Corrupted script header: missing script comment.", out.str ().c_str ());
	}
	if ((_scriptKind.IsSetChange () || _scriptKind.IsUnitChange ()) && !_lineage.Verify ())
	{
		out << *this;
		throw Win::Exception ("Corrupted script header: illegal script main lineage.", out.str ().c_str ());
	}
	for (unsigned int idx = 0; idx < _sideLineages.size (); ++idx)
	{
		UnitLineage const * lineage = _sideLineages [idx];
		if (!lineage->Verify ())
		{
			out << *this;
			throw Win::Exception ("Corrupted script header: illegal script side lineage.", out.str ().c_str ());
		}
	}
}

void ScriptHeader::Init ()
{
	if (_scriptKind.IsData ())
	{
		if (_scriptKind.IsCumulative ())
		{
			Assert (_unitId == gidInvalid);
			_unitType = Unit::Set;
		}
		else
		{
			Assert (_unitId != gidInvalid);
			_unitType = Unit::Member;
		}
	}
	else
	{
		Assert (_scriptKind.IsControl ());
		Assert (_unitId == gidInvalid);
		_unitType = Unit::Ignore;
	}
	_timeStamp = CurrentTime ();
}

void ScriptHeader::Clear ()
{
	_partNumber = 1;
	_partCount = 1;
	_maxChunkSize = 0;
    _comment.clear ();
    _projectName.clear ();
	_scriptId = gidInvalid;
    _scriptKind = ScriptKind ();
	_unitType = Unit::Set;
	_unitId = gidInvalid;
    _timeStamp = 0;
    _lineage.Clear ();
	_sideLineages.clear ();
	_isFromVersion40 = false;
	_isVersion40EmergencyAdminElection = false;
}

std::ostream & operator<<(std::ostream & os, ScriptHeader const & hdr)
{
	GlobalIdPack pack (hdr.ScriptId ());
	os << "Script id: " << pack.ToSquaredString () << "; Project: " << hdr.GetProjectName () << std::endl;
	if (hdr.IsChunk ())
		os << std::dec << hdr.GetPartNumber () << " of " << hdr.GetPartCount () << "; Max chunk size: " << hdr.GetMaxChunkSize () << " bytes" << std::endl;
	os << "Script kind: " << hdr.GetScriptKind () << "; Type: " << hdr.GetUnitType ();
	if (hdr.GetUnitType () != Unit::Set)
		os << "; Unit id: " << GlobalIdPack (hdr.GetModifiedUnitId ()).ToString ();
	os << std::endl;
	os << "Main lineage: " << hdr.GetLineage () << std::endl;
	ScriptHeader::SideLineageSequencer seq (hdr);
	if (!seq.AtEnd ())
	{
		os << "Side lineage(s):" <<std::endl;
		for ( ; !seq.AtEnd (); seq.Advance ())
		{
			UnitLineage const & unitLineage = seq.GetLineage ();
			os << "   " << unitLineage << std::endl;
		}
	}
	if (hdr.IsFromVersion40 ())
		os << "Converted version 4.0 script" << std::endl;
	if (hdr.IsVersion40EmergencyAdminElection ())
		os << "Converted version 4.0 emergency administrator election script" << std::endl;
	os << "Comment: " << hdr.GetComment ();
	return os;
}
