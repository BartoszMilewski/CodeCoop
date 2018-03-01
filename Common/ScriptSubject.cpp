//---------------------------
// (c) Reliable Software 2001
//---------------------------

#include "precompiled.h"
#include "ScriptSubject.h"
#include <Dbg/Assert.h>
#include <sstream>

namespace Subject
{
	Parser::Parser (std::string const & subject)
		: _subject (subject), _ordinal (1), _count (1), _isSimple (false), _isOk (false)
	{
		char const stdSubject [] = "Code Co-op Synch Scripts";
		if (_subject == stdSubject)
		{
			_isOk = true;
			_isSimple = true;
		}
		else if (_subject.compare (0, sizeof (PREFIX) - 1, PREFIX) == 0)
		{
			// Code Co-op Sync:znc:1-of-5:My Project:7-f82
			size_t beg = sizeof (PREFIX) - 1;
			size_t end = _subject.find (':', beg);
			if (end == std::string::npos)
			{
				// Code Co-op Sync:znc
				_extension = _subject.substr (beg, _subject.size () - beg);
				_isOk = true;
			}
			else
			{
				_extension = _subject.substr (beg, end - beg);
				// Ordinal and Count
				// :x-of-y: or empty ::
				std::istringstream in (_subject.substr (end));
				char c;
				in >> c; // opening ':'
				if (c != ':')
					return; // malformed
				in >> c; // closing ':' or ordinal/count
				if (c != ':')
				{
					in.putback (c);
					in >> _ordinal;
					char o, f, d;
					in >> c >> o >> f >> d;
					if (c != '-' || o != 'o' || f != 'f' || d != '-')
						return; // malformed
					in >> _count;
					in >> c; // closing ':'
					if (c != ':')
						return; // malformed
				}
				// the rest is script id: (project name ':' and script id)
				// notice: project name may be any string
				std::getline (in, _scriptId);
				_isOk = true;
			}
			// retrieve original extension as in pgp.znc
			beg = _extension.find ('.');
			if (beg != std::string::npos)
			{
				size_t len = _extension.size () - beg - 1;
				_originalExtension = _extension.substr (beg + 1, len);
				_extension = _extension.substr (0, beg);
			}
		}
	}
	
	Maker::Maker (std::string const & extension, 
				  std::string const & project, 
				  GlobalId scriptId,
				  std::string const & origExt)
		: _subject (PREFIX)
	{
		_subject += extension;
		if (!origExt.empty ())
		{
			_subject += '.';
			_subject += origExt;
		}
		_subject += ':';
		// Code Co-op Sync:znc:

		_scriptId += project;
		_scriptId += ':';
		GlobalIdPack gidPack (scriptId);
		_scriptId += gidPack.ToString ();
		// MyProject:1-2d0
	}

	std::string const & Maker::Get (unsigned partNumber, unsigned partCount)
	{
		Assert (partNumber <= partCount);
		std::ostringstream out;
		if (partCount != 1)
		{
			out << partNumber << "-of-" << partCount;
			_subject += out.str ();
		}
		_subject += ':';
		_subject += _scriptId;
		return _subject;
	}
}

