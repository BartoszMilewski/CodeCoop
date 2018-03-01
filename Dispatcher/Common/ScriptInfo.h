#if !defined (DISPATCHER_H)
#define DISPATCHER_H
//----------------------------------
// (c) Reliable Software 1998 - 2008
// ---------------------------------

#include "Address.h"
#include "Addressee.h"
#include "HubId.h"
#include "ScriptStatus.h"
#include "GlobalId.h"
#include "Transport.h"
#include <StringOp.h>
#include <Bit.h>

class ScriptStatus;
class ScriptTicket;

class DispatchRequest
{
	friend std::ostream& operator<<(std::ostream& os, DispatchRequest const & r);
public:
	DispatchRequest () {} // don't use it: needed by the map

	DispatchRequest (ScriptTicket * script, int recipIdx, Transport const & transport)
		: _script (script), 
		  _recipIdx (recipIdx),
		  _transport (transport), 
		  _tried (false),
		  _delivered (false)
	{}
	bool IsForward () const { return !_transport.IsUnknown () && !_transport.IsEmail (); }
	bool IsFwdEmail () const { return _transport.IsEmail (); }
	Transport const & GetTransport () const { return _transport; }

	void MarkTried (bool val = true) { _tried = val; }
	bool WasTried () const { return _tried; }
	void MarkDelivered (bool val = true) { _delivered = val; }
	bool IsDelivered () const { return _delivered; }
public:
	ScriptTicket *	_script;
	int				_recipIdx;
	Transport		_transport;
	bool			_tried;		// may be still marked for retry
	bool			_retry;
	bool			_delivered;
};

std::ostream& operator<<(std::ostream& os, DispatchRequest const & r);

// Immutable object, can be freely shared between threads
class ScriptInfo
{
	friend class ScriptTicket;
	friend std::ostream& operator<<(std::ostream& os, ScriptInfo const & script);
public:
	class RecipientInfo;
	typedef std::multimap<int, DispatchRequest>::iterator RequestIter;
	typedef std::multimap<int, DispatchRequest>::const_iterator ConstRequestIter;
public:

    ScriptInfo (FilePath const & mailboxPath, std::string name); 
    
    bool HasHeader () const { return _flags.test (HeaderPresent); }
    bool IsHeaderValid () const { return _flags.test (ValidHeader); }
    bool ToBeFwd () const { return _flags.test (ToBeForwarded); }
	bool IsDefect () const { return _flags.test (DefectScript); }
	bool IsControl () const { return _flags.test (ControlScript); }
    bool HasDispatcherAddendum () const { return _flags.test (DispatcherAddendum); }
	bool UseBccRecipients () const { return _flags.test (BccRecipients); }
	bool RemovesMember (std::string const & hubId, std::string const & userId);
	bool IsFromThisCluster (std::string const & thisHubId) const;

	std::string const & GetName () const { return _fileName; }
	std::string const & GetPath() const { return _fullPath; }
	std::string const & GetComment () const { return _comment; }
	Address const & GetSender () const { return _senderAddress; }
	std::string const & GetProjectName () const { return _senderAddress.GetProjectName (); }
	std::string const & GetSenderHubId () const { return _senderAddress.GetHubId (); }
	std::string const & GetSenderUserId () const { return _senderAddress.GetUserId (); }
	File::Size GetSize () const { return _size; }
	GlobalId GetScriptId () const { return _scriptId; }
	unsigned GetPartNumber () const { return _partNumber; }
	unsigned GetPartCount () const { return _partCount; }
	unsigned GetMaxChunkSize () const { return _maxChunkSize; }
    int GetAddresseeCount() const { return _recipients.size (); }
	RecipientInfo const & GetAddressee(int i) const { return _recipients [i]; }

	bool IsInvitation () const { return _flags.test (Invitation); }

	bool operator== (ScriptInfo const & script) const;
public:
	class RecipientInfo
	{
	public:
		RecipientInfo(std::string const & hubId, std::string const & userId, bool isDelivered)
			: _hubId(hubId), _userId(userId), _isDelivered(isDelivered)
		{}
		std::string const & GetHubId() const { return _hubId; }
		std::string const & GetUserId() const { return _userId; }
		std::string const & GetDisplayUserId () const 
		{
			static const std::string admin ("\"project administrator\"");
			if (HasWildcardUserId ())
			{
				return admin;
			}
			return _userId;
		}
		bool HasWildcardUserId () const
		{
			return _userId.compare ("*") == 0;
		}
		bool IsHubDispatcher () const
		{
			return GetUserId () == DispatcherAtHubId;
		}
		bool IsSatDispatcher () const
		{
			return GetUserId () == DispatcherAtSatId;
		}
		bool IsDispatcher () const 
		{
			return IsHubDispatcher () || IsSatDispatcher ();
		}
		bool IsDelivered() const { return _isDelivered; }
	private:
		std::string _hubId;
		std::string _userId;
		bool		_isDelivered;
	};
private:
	enum ScriptFlags
	{
		HeaderPresent,
		ValidHeader,
		ToBeForwarded, // fwd flag in script header
		DefectScript,
		ControlScript,
		DispatcherAddendum,
		BccRecipients,
		Invitation,
	};

private:
	BitSet<ScriptFlags>	_flags;
    std::string const	_fileName;
	File::Size			_size;
	std::string			_fullPath;

	GlobalId			_scriptId;
	Address				_senderAddress;
	// Revisit: use a vector of hubId, userId
	std::vector<RecipientInfo> _recipients;

	std::string			_comment;
	unsigned			_partNumber;	// for chunks
	unsigned			_partCount;
	unsigned			_maxChunkSize;
};

class ScriptTicket
{
	friend std::ostream& operator<<(std::ostream& os, ScriptTicket const & script);
public:
	typedef std::multimap<int, DispatchRequest>::iterator RequestIter;
	typedef std::multimap<int, DispatchRequest>::const_iterator ConstRequestIter;
public:
    ScriptTicket (ScriptInfo const & info);
    
    bool HasHeader() const { return _info.HasHeader(); }
    bool IsHeaderValid() const { return _info.IsHeaderValid(); }
    bool ToBeFwd() const { return _info.ToBeFwd(); }
	bool IsDefect() const { return _info.IsDefect(); }
	bool IsControl() const { return _info.IsControl(); }
    bool HasDispatcherAddendum() const { return _info.HasDispatcherAddendum(); }
	bool UseBccRecipients() const { return _info.UseBccRecipients(); }
	bool ScriptTicket::RemovesMember(std::string const & hubId, std::string const & userId);
	bool IsFromThisCluster(std::string const & thisHubId) const
	{
		return _info.IsFromThisCluster(thisHubId);
	}

	ScriptInfo const & GetInfo() const { return _info; }
    char const * GetName() const { return _info._fileName.c_str (); }
	std::string const & GetComment() const { return _info._comment; }
    char const * GetPath() const { return _info.GetPath().c_str (); }
	Address const & GetSender() const { return _info._senderAddress; }
	std::string const & GetProjectName() const { return _info._senderAddress.GetProjectName (); }
	std::string const & GetSenderHubId() const { return _info._senderAddress.GetHubId (); }
	std::string const & GetSenderUserId() const { return _info._senderAddress.GetUserId (); }
	File::Size GetSize() const { return _info._size; }
	GlobalId GetScriptId() const { return _info._scriptId; }
    int GetAddresseeCount() const { return _info.GetAddresseeCount(); }
	ScriptInfo::RecipientInfo const & GetAddressee(int i) const { return _info.GetAddressee(i); }
	unsigned GetPartNumber() const { return _info._partNumber; }
	unsigned GetPartCount() const { return _info._partCount; }
	unsigned GetMaxChunkSize() const { return _info._maxChunkSize; }

	bool IsInvitation() const { return _info.IsInvitation(); }
	void SetEmailError() { _flags.set (EmailError); }
	bool IsEmailError() const { return _flags.test (EmailError); }
	void SetIsPostponed() { _flags.set (Postponed); }
	bool IsPostponed() const { return _flags.test (Postponed); }
	std::string const & GetEmail(ScriptTicket::RequestIter ri) const;

	void MarkAddressingError(int idx);
    void MarkPassedOver(int idx);
	void MarkDeliveredToAnyone() { _flags.set (DeliveredToAnyone, true); }
    void StampDelivery(int idx);
    void StampFinalDelivery(int idx);
	bool operator==(ScriptTicket const & script) const;
	// Should be removed
	bool operator==(ScriptInfo const & info) const
	{
		return GetInfo() == info;
	}
	bool CanDelete() const;
    bool DeliveredToAll() const;
    bool DeliveredToAnyoneOnThisMachine() const { return _flags.test (DeliveredToAnyone); }
	bool HasUnknownNotStampedRecipients() const;
	bool IsStamped(int idx) const { return _stampedRecipients[idx]; }
	bool IsPassedOver(int idx) const;
	bool IsAddressingError(int idx) const;

	void AddRequest(int recipIdx, Transport const & transport);
	void MarkEmailReqDone(std::vector<std::string> const & addresses, bool isSuccess);
	void MarkRequestDone(Transport const & transport, bool isSuccess);
	bool AreRequestsPending() const;
	bool AreForwardRequestsPending () const;

	void RefreshRequests();
	// request iteration
	RequestIter beginReq() { return _requests.begin (); }
	RequestIter endReq()   { return _requests.end (); }
	int EmailReqCount() const { return _emailReqCount; }
	int FwdReqCount() const { return _fwdReqCount; }
	// should always be true
	bool RequestsTally() const;

private:
	void StampRecipient(int idx);
	bool AllRequestsDeliveredTo(int idx);
private:
	enum ScriptFlags
	{
		DeliveredToAnyone,
		EmailError,
		Postponed
	};

	enum RecipientFlags
	{
		Unknown,
		PassedOver,
		Error
	};

public:
	// Immutable
	ScriptInfo _info;

private:
	BitSet<ScriptFlags>	_flags;

	// vector parallel to _recipients in _info
	std::vector<RecipientFlags>	_recipientFlags;
	std::vector<bool>			_stampedRecipients;

	// Table of dispatch requests keyed by recipient index
	// Note: there may be more requests than recipients,
	// for instance when broadcasting to all satellites
	std::multimap<int, DispatchRequest> _requests;
	int						_emailReqCount;
	int						_fwdReqCount;
};

struct ScriptAbstract
{
	ScriptAbstract() {}
	ScriptAbstract(ScriptInfo const & script)
		: _scriptPath(script.GetPath()),
		_comment(script.GetComment()),
		_projectName(script.GetProjectName()),
		_scriptId(script.GetScriptId()),
		_partNumber(script.GetPartNumber()),
		_partCount(script.GetPartCount()),
		_useBccRecipients(script.UseBccRecipients()),
		_isControl(script.IsControl()),
		_toBeFwd(script.ToBeFwd())
	{}
	
	std::string _scriptPath;
	std::string _comment;
	std::string _projectName;
	GlobalId _scriptId;
	unsigned _partNumber;
	unsigned _partCount;
	bool _useBccRecipients;
	bool _isControl;
	bool _toBeFwd;
};


typedef auto_vector<ScriptTicket> ScriptVector;

class IsEqualScript : public std::unary_function<ScriptTicket const *, bool>
{
public:
	IsEqualScript(ScriptTicket const & script) : _script(script) {}
	bool operator()(ScriptTicket const * st)
	{
		return *st == _script;
	}
private:
	ScriptTicket const & _script;
};

std::ostream& operator<<(std::ostream& os, ScriptTicket const & script);

#endif
