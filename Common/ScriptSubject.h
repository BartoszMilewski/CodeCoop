#if !defined (SCRIPTSUBJECT_H)
#define SCRIPTSUBJECT_H
//---------------------------
// (c) Reliable Software 2001
//---------------------------
#include "GlobalId.h"

namespace Subject
{
	char const PREFIX [] = "Code Co-op Sync:";

	class Parser
	{
	public:
		Parser (std::string const & subject);
		bool IsScript () const 
		{ 
			return _isOk && 
				   (IsSimple () || GetCount () == 1 || !GetOriginalExtension ().empty ());
		}
		bool IsSimple () const { return _isSimple; }
		std::string const & GetExtension () const { return _extension; }
		std::string const & GetOriginalExtension () const { return _originalExtension; }
		std::string const & GetScriptId () const { return _scriptId; }
		unsigned GetCount () const { return _count; }
		unsigned GetOrdinal () const { return _ordinal; }
	private:
		std::string const & _subject;
		bool _isOk;
		bool _isSimple;
		std::string _extension;
		std::string _originalExtension;
		std::string _scriptId;
		unsigned	_ordinal;
		unsigned	_count;
	};

	class Maker
	{
	public:
		// Code Co-op Sync:pgp.znc:1-of-1:My Project:7-f82
		Maker (std::string const & extension, 
				std::string const & project, 
				GlobalId scriptId,
				std::string const & origExt = std::string ());
		std::string const & Get (unsigned partNumber, unsigned partCount);
	private:
		std::string _subject;
		std::string	_scriptId;
		unsigned	_parts;
	};

}

#endif
