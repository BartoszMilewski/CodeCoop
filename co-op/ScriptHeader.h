#if !defined (SCRIPTHEADER_H)
#define SCRIPTHEADER_H
//------------------------------------
//  (c) Reliable Software, 2003 - 2007
//------------------------------------

#include "Params.h"
#include "GlobalId.h"
#include "ScriptSerialize.h"
#include "ScriptKind.h"
#include "Lineage.h"
#include "SerString.h"

#include <auto_vector.h>

#include <iosfwd>

class ScriptHeader : public ScriptSerializable
{
public:
	class SideLineageSequencer
	{
	public:
		SideLineageSequencer (ScriptHeader const & hdr)
			: _cur (hdr._sideLineages.begin ()),
			  _end (hdr._sideLineages.end ())
		{}

		bool AtEnd () const { return _cur == _end; }
		void Advance () { ++_cur; }

		UnitLineage const & GetLineage () const { return **_cur; }

	private:
		auto_vector<UnitLineage>::const_iterator	_cur;
		auto_vector<UnitLineage>::const_iterator	_end;
	};

	friend class SideLineageSequencer;

public:
	ScriptHeader ()
		: _scriptId (gidInvalid),
		  _unitType (Unit::Set),
		  _unitId (gidInvalid),
		  _timeStamp (0),
		  _isDeepFork (false),
		  _isFromVersion40 (false),
		  _isVersion40EmergencyAdminElection (false),
		  _partNumber (1),
		  _partCount (1),
		  _maxChunkSize (0)
    {}
    ScriptHeader (Deserializer& in)
		: _scriptId (gidInvalid),
		  _unitType (Unit::Set),
		  _unitId (gidInvalid),
		  _timeStamp (0),
		  _isDeepFork (false),
		  _isFromVersion40 (false),
		  _isVersion40EmergencyAdminElection (false),
		  _partNumber (1),
		  _partCount (1),
		  _maxChunkSize (1)
	{
		Read (in);
	}
	ScriptHeader (ScriptKind kind, GlobalId modifiedUnitId, std::string const & projectName)
		: _scriptId (gidInvalid),
		  _projectName (projectName),
		  _scriptKind (kind),
		  _unitId (modifiedUnitId),
		  _timeStamp (0),
		  _isDeepFork (false),
		  _isFromVersion40 (false),
		  _isVersion40EmergencyAdminElection (false),
		  _partNumber (1),
		  _partCount (1),
		  _maxChunkSize (0)
	{
		Init ();
	}
	void SetScriptId (GlobalId scriptId)
	{
		_scriptId = scriptId;
	}

	void CopyChunkInfo (ScriptHeader const & hdr);
    void SetProjectName (std::string const & projectName) { _projectName = projectName; }
    void AddComment (std::string const & comment) { _comment = comment; }
	void AddScriptId (GlobalId id) { _scriptId = id; }
    void AddTimeStamp (unsigned long timeValue) { _timeStamp = timeValue; }
	void AddMainLineage (Lineage const & lineage) { _lineage.ReInit (lineage); }
	void AddSideLineage (std::unique_ptr<UnitLineage> sideLineage) { _sideLineages.push_back (std::move(sideLineage)); }
	void SwapSideLineages (auto_vector<UnitLineage> & sideLineages)
	{ 
		_sideLineages.swap (sideLineages); 
	}
	void SetScriptKind (ScriptKind kind) { _scriptKind = kind; }
	void SetUnitType (Unit::Type type) { _unitType = type; }
	void SetModifiedUnitId (GlobalId id) { _unitId = id; }
	void SetChunkInfo (unsigned partNumber, unsigned partCount, unsigned maxChunkSize)
	{
		_partNumber = partNumber;
		_partCount = partCount;
		_maxChunkSize = maxChunkSize;
	}
	void SetDeepFork (bool flag) { _isDeepFork = flag; }
	void SetVersion40 (bool flag) { _isFromVersion40 = flag; }
	void SetVersion40EmergencyAdminElection (bool flag) { _isVersion40EmergencyAdminElection = flag; }
	void Clear ();

	std::string const & GetProjectName () const { return _projectName; }
	std::string const & GetComment () const { return _comment; }
	unsigned GetPartNumber () const { return _partNumber; }
	unsigned GetPartCount () const { return _partCount; }
	unsigned GetMaxChunkSize () const { return _maxChunkSize; }
    long GetTimeStamp () const { return _timeStamp; }
    GlobalId ScriptId () const { return _scriptId; }
    ScriptKind GetScriptKind () const { return _scriptKind; }
	Unit::Type GetUnitType () const { return _unitType; }
	GlobalId GetModifiedUnitId () const { return _unitId; }
    Lineage const & GetLineage () const { return _lineage; }
	void SwapMainLineage (Lineage & lineage) { lineage.swap (_lineage); }
    UserId SenderUserId () const 
    { 
        GlobalIdPack gid (ScriptId ()); 
        return gid.GetUserId ();
    }

	bool IsChunk () const { return _partCount != 1 && !IsResendRequest (); }
	bool IsFullSynch () const { return _scriptKind.IsFullSynch (); }
	bool IsPackage () const { return _scriptKind.IsRegularPackage (); }
	bool IsData () const { return _scriptKind.IsData (); }
    bool IsSetChange () const { return _scriptKind.IsSetChange (); }
    bool IsControl () const { return _scriptKind.IsControl (); }
    bool IsPureControl () const { return _scriptKind.IsPureControl (); }
	bool IsAck () const { return _scriptKind.IsAck (); }
	bool IsMembershipChange () const { return _scriptKind.IsMergeable (); }
	bool IsAddMember () const { return _scriptKind.IsAddMember (); }
	bool IsEditMember () const { return _scriptKind.IsEditMember (); }
	bool IsDefectOrRemove () const { return _scriptKind.IsDeleteMember (); }
	bool IsJoinRequest () const { return _scriptKind.IsJoinRequest (); }
	bool IsScriptResendRequest () const { return _scriptKind.IsScriptResendRequest (); }
	bool IsFullSynchResendRequest () const { return _scriptKind.IsFullSynchResendRequest (); }
	bool IsResendRequest () const { return IsScriptResendRequest () || IsFullSynchResendRequest (); }
	bool IsVerificationPackage () const { return _scriptKind.IsVerificationPackage (); }
	bool IsDeepFork () const { return _isDeepFork; }
	bool IsFromVersion40 () const { return _isFromVersion40; }
	bool IsVersion40EmergencyAdminElection () const { return _isVersion40EmergencyAdminElection; }

	// Any changes to data layout must be propagated
	// to ScriptSubHeader class serialization !
    bool IsSection () const { return true; }
    int  SectionId () const { return 'SHDR'; }
    int  VersionNo () const { return scriptVersion; }
	void Serialize (Serializer& out) const;
    void Deserialize (Deserializer& in, int version);
	void Verify () const throw (Win::Exception);

private:
	void Init ();

private:
	unsigned					_partNumber;	// for chunks
	unsigned					_partCount;
	unsigned					_maxChunkSize;
    SerString					_comment;
    SerString					_projectName;
	GlobalId					_scriptId;
    ScriptKind					_scriptKind;
	Unit::Type					_unitType;
	GlobalId					_unitId;
    long						_timeStamp;
    Lineage						_lineage;
	auto_vector<UnitLineage>	_sideLineages;
	// Volatile
	bool						_isDeepFork;
	bool						_isFromVersion40;
	bool						_isVersion40EmergencyAdminElection;
};

std::ostream & operator<<(std::ostream & os, ScriptHeader const & hdr);

#endif
