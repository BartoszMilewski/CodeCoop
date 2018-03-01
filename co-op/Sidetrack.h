#if !defined (SIDETRACK_H)
#define SIDETRACK_H
//-----------------------------------
// (c) Reliable Software, 2004 - 2005
//-----------------------------------

#include "Transactable.h"
#include "GlobalId.h"
#include "Params.h"
#include "Lineage.h"
#include "SerDateTime.h"
#include "SerVector.h"
#include "SerString.h"
#include "XArray.h"

class ScriptHeader;
class CommandList;
class PathFinder;
namespace Project { class Db; }
class FullSynchData;
class ScriptBasket;
class AddresseeList;

class Sidetrack: public TransactableContainer
{
public:
	class Missing;
	class MissingChunk;
	enum { npos = 0xffffffff };
private:
	class ResendRequest;
	typedef TransactableArray<Missing>::const_iterator MissingIter;
	typedef TransactableArray<MissingChunk>::const_iterator MissingChunkIter;
	typedef TransactableArray<ResendRequest>::const_iterator ResendRequestIter;
public:
	Sidetrack (Project::Db & projectDb, PathFinder const & pathFinder);
	bool HasChunks () const { return _missingChunks.Count () != 0; }
	bool IsEmpty () const { return (_missing.Count () == 0) && (_missingChunks.Count () == 0); }
	bool IsNewMissing_Reset ();
	unsigned XAddChunk (ScriptHeader const & inHdr, std::string const & fileName);
	bool XRememberResendRequest (ScriptHeader const & inHdr, CommandList const & cmdList);
	bool NeedsUpdating (Unit::ScriptList const & historyMissingScripts) const;
	bool HasFullSynchRequests () const;
	bool GetFullSynchRequest (FullSynchData & fullSynchData,
							  std::vector<unsigned> & chunkNumbers,
							  unsigned & maxChunkSize,
							  unsigned & chunkCount);
	void GetMissingScripts (Unit::Type unitType, GidSet & gids) const;
	Missing const * GetMissing (GlobalId scriptId) const;
	bool IsMissing (GlobalId gid) const;
	bool IsMissingChunked (GlobalId gid, unsigned & received, unsigned & total) const;
	UserId SentTo (GlobalId gid) const;
	UserId NextRecipientId (GlobalId) const;
	void XRemoveMissingScript (GlobalId gid);
	void XRemoveFullSynchChunkRequest (GlobalId scriptId = gidInvalid);

	bool XProcessMissingScripts (Unit::ScriptList const & missingScripts, ScriptBasket & scriptBasket);
	void XRequestResend(GlobalId scriptId, UserIdList const & addresseeList, ScriptBasket & scriptBasket);
	bool NeedsUpdate (GlobalId scriptId) const;
	void Dump (std::ostream  & out) const;

	// TransactableContainer
	void Serialize (Serializer& out) const 
	{
		_missing.Serialize (out);
		_missingChunks.Serialize (out);
		_resendRequests.Serialize (out);
	}
	void Deserialize (Deserializer& in, int version)
	{
		_missing.Deserialize (in, version);
		if (version >= 47)
			_missingChunks.Deserialize (in, version);
		if (version >= 49)
			_resendRequests.Deserialize (in, version);
	}
	bool IsSection () const { return true; }
	int  SectionId () const { return 'SIDE'; }
	int  VersionNo () const { return modelVersion; }

private:
	Missing * XNeedsScriptResend (GlobalId scritpId, Unit::Type unitType);
	MissingChunk * XNeedsChunkResend (GlobalId scriptId, bool & isFound);
	void XSendScriptRequest (Sidetrack::Missing * resendEntry, ScriptBasket & scriptBasket);
	void XSendChunkRequest (Sidetrack::MissingChunk * resendEntry, ScriptBasket & scriptBasket);
	void XSendFullSynchChunkRequest (Sidetrack::MissingChunk * resendEntry, ScriptBasket & scriptBasket);
	void XRemoveMissingChunk (unsigned idx);
	unsigned XFindChunkEntry(GlobalId scriptId);
	unsigned XFindScriptEntry(GlobalId scriptId);
	void XMakeScriptRequest(Sidetrack::Missing const * resendEntry, 
							UserIdList const & userIds, 
							ScriptBasket & scriptBasket);
	void XMakeChunkRequest(Sidetrack::MissingChunk const * resendEntry, 
							UserIdList const & userIds, 
							ScriptBasket & scriptBasket);
private:
	bool mutable					_isNewMissing;
	Project::Db &					_projectDb;
	PathFinder const &				_pathFinder;
	TransactableArray<Missing>		_missing;
	TransactableArray<MissingChunk>	_missingChunks;
	TransactableArray<ResendRequest>_resendRequests;
};

class Sidetrack::Missing: public Serializable
{
public:
	Missing () : _scriptId (gidInvalid), _unitType (Unit::Ignore) {}
	Missing (GlobalId scriptId, Unit::Type unitType, Project::Db & projectDb);
	Missing (Deserializer& in, int version) { Deserialize (in, version); }
	GlobalId ScriptId () const { return _scriptId; }
	Unit::Type UnitType () const { return _unitType; }
	PackedTime const & NextTime () const { return _nextTime; }
	virtual bool IsFullSynch () const { return false; }
	virtual void Update (Project::Db & projectDb);
	UserId NextRecipientId () const { return _recipients.back (); }
	bool IsExhausted () const { return _recipients.back () == gidInvalid; }
	std::vector<UserId> const & GetRequestRecipients () const { return _recipients; }
	UserId SentTo () const;
	void ResetRecipients ();
	// Serializable
	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);
protected:
	void SelectRecipient (Project::Db & projectDb);
protected:
	GlobalId		_scriptId;
	Unit::Type		_unitType;
	SerPackedTime	_nextTime;
	// History of requests + the next request on top
	SerVector<UserId>	_recipients;
};

class Sidetrack::MissingChunk: public Missing
{
public:
	// part number -> file name
	typedef std::map<unsigned, std::string> PartMap;
	enum { bitFullSynch = 0, bitCount };
public:
	MissingChunk (ScriptHeader const & inHdr, Project::Db & projectDb);
	MissingChunk (Deserializer& in, int version) { Deserialize (in, version); }
	bool IsFullSynch () const { return _flags.test (bitFullSynch); }
	PartMap const & GetPartMap () const { return _received; }
	PartMap & GetPartMap () { return _received; }
	virtual void Update (Project::Db & projectDb);
	void StoreChunk (unsigned partNumber, std::string const & fileName, PathFinder const & pathFinder);
	unsigned GetPartCount () const { return _partCount; }
	unsigned GetStoredCount () const { return _received.size (); }
	unsigned GetMaxChunkSize () const { return _maxChunkSize; }
	void Reconstruct (PathFinder const & pathFinder);
	// Serializable
	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);
private:
	unsigned	_partCount;
	unsigned	_maxChunkSize;
	PartMap		_received;
	std::bitset<bitCount> _flags;
};

class Sidetrack::ResendRequest: public Serializable
{
	enum { bitCount = 1 };
public:
	ResendRequest (GlobalId scriptId, unsigned partCount, unsigned maxChunkSize, std::string const & path)
		: _scriptId (scriptId), _path (path), _partCount (partCount), _maxChunkSize (maxChunkSize)
	{}
	ResendRequest () : _scriptId (gidInvalid) {}
	ResendRequest (Deserializer& in, int version) { Deserialize (in, version); }
	void AddChunkNo (unsigned chunkNo);
	GlobalId ScriptId () const { return _scriptId; }
	unsigned GetMaxChunkSize () const { return _maxChunkSize; }
	unsigned GetPartCount () const { return _partCount; }
	std::vector<unsigned> const & GetChunkNumbers () const { return _chunkNumbers; }
	std::string const & GetPath () const { return _path; }
	// Serializable
	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);
protected:
	GlobalId	_scriptId;
	std::bitset<bitCount> _flags;
	SerString	_path;
	unsigned	_partCount;
	unsigned	_maxChunkSize;
	SerVector<unsigned>	_chunkNumbers;
};

class IsEqualId
{
public:
	IsEqualId (GlobalId gid) : _gid (gid) {}
	bool operator () (Sidetrack::Missing const * missing)
	{
		return missing->ScriptId () == _gid;
	}
private:
	GlobalId _gid;
};

std::ostream & operator<<(std::ostream & os, Sidetrack::Missing const & missingScript);
std::ostream & operator<<(std::ostream & os, Sidetrack::MissingChunk const & missingChunk);

#endif
