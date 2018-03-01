#if !defined (MAILBOXSCRIPTINFO_H)
#define MAILBOXSCRIPTINFO_H
//------------------------------------
//  (c) Reliable Software, 2005 - 2006
//------------------------------------

#include "GlobalId.h"
#include "Lineage.h"
#include "ScriptKind.h"
#include "MailboxScriptState.h"

#include <iosfwd>

class ScriptHeader;

namespace Mailbox
{
	class ScriptInfo
	{
	public:
		ScriptInfo (ScriptState state, std::string const & path, ScriptHeader const * hdr);
		ScriptInfo (ScriptState state, std::string const & path);

		void SetErrorMsg (std::string const & msg) { _errorMsg.assign (msg); }

		std::string const & GetScriptPath () const { return _path; }
		std::string const & Caption () const { return _caption; }
		std::string const & GetTimeStamp () const { return _timeStamp; }
		std::string const & GetErrorMsg () const { return _errorMsg; }
		GlobalId GetScriptId () const { return _id; }
		GlobalId GetReferenceId () const{ return _referenceId; }
		Unit::Type GetUnitType () const { return _unitType; }
		ScriptState GetScriptState () const { return _state; }
		ScriptKind GetScriptKind () const { return _kind; }

		bool IsSetChange () const {	return _kind.IsSetChange (); }
		bool IsControl () const	{ return _kind.IsControl ();	}
		bool IsJoinRequest () const { return _kind.IsJoinRequest (); }
		bool IsFullSynch () const { return _kind.IsFullSynch (); }
		bool IsFullSynchResendRequest () const { return _kind.IsFullSynchResendRequest (); }
		bool IsForThisProject () const { return _state.IsForThisProject (); }
		bool IsCorrupted () const { return _state.IsCorrupted (); }
		bool IsIllegalDuplicate () const { return _state.IsIllegalDuplicate (); }
		bool IsFromFuture () const { return _state.IsFromFuture (); }
		bool IsNext () const { return _state.IsNext (); }
		bool IsCannotForward () const { return _state.IsCannotForward (); }

	private:
		GlobalId	_id;
		GlobalId	_referenceId;
		Unit::Type	_unitType;
		ScriptKind	_kind;
		std::string	_caption;	// First comment line
		std::string	_timeStamp;
		std::string	_path;
		ScriptState _state;
		std::string	_errorMsg;
	};
}

std::ostream & operator<<(std::ostream & os, Mailbox::ScriptInfo const & info);

#endif
