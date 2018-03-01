//------------------------------------
//  (c) Reliable Software, 2005 - 2006
//------------------------------------

#include "precompiled.h"
#include "MailboxScriptInfo.h"
#include "ScriptHeader.h"
#include "ScriptCommandList.h"
#include "MultiLine.h"
#include "ScriptIo.h"
#include "OutputSink.h"

#include "TimeStamp.h"

using namespace Mailbox;

ScriptInfo::ScriptInfo (ScriptState state, std::string const & path, ScriptHeader const * hdr)
	: _id (hdr->ScriptId ()),
	  _referenceId (hdr->GetLineage ().GetReferenceId ()),
	  _unitType (hdr->GetUnitType ()),
	  _kind (hdr->GetScriptKind ()),
	  _path (path),
	  _state (state)
{
	Assert (hdr != 0);
	MultiLineComment comment (hdr->GetComment ());
	_caption.assign (comment.GetFirstLine ());
	StrTime timeStamp (hdr->GetTimeStamp ());
	_timeStamp.assign (timeStamp.GetString ());
}

ScriptInfo::ScriptInfo (ScriptState state, std::string const & path)
	: _id (gidInvalid),
	  _unitType (Unit::Ignore),
	  _path (path),
	  _state (state),
	  _caption ("Corrupted script cannot be processed")
{}

std::ostream & operator<<(std::ostream & os, Mailbox::ScriptInfo const & info)
{
	os << info.GetScriptPath () << std::endl;
	try
	{
		ScriptReader reader (info.GetScriptPath ().c_str ());
		std::unique_ptr<ScriptHeader> hdr = reader.RetrieveScriptHeader ();
		os << *hdr << std::endl;
	}
	catch (Win::Exception ex)
	{
		os << Out::Sink::FormatExceptionMsg (ex) << std::endl;
	}
	catch ( ... )
	{
		os << "Script header cannot be read." << std::endl;
		os << info.Caption () << std::endl;
	}
	os << "State: ";
	if (!info.IsForThisProject ())
		os << "not for this project";
	else if (info.IsCorrupted ())
		os << "corrupted";
	else if (info.IsFromFuture ())
		os << "from the future";
	else if (info.IsIllegalDuplicate ())
		os << "illegal duplicate";
	if (info.IsJoinRequest ())
		os << "join request";
	if (info.IsNext ())
		os << ", NEXT";
	os << ", " << info.GetErrorMsg ();
	return os;
}
