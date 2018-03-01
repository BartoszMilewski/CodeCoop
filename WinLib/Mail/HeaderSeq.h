#if !defined (HEADER_SEQ_H)
#define HEADER_SEQ_H
// ---------------------------------
// (c) Reliable Software 2003 - 2005
// ---------------------------------

#include <Parse/NamedPair.h>
#include <Parse/BufferedStream.h>
#include <StringOp.h>

// Email message header sequencer
// Understands general syntax of message headers
// Each header is parsed into
//     name: value; comment
// Line folding is taken into account
// Empty line is considered end of header list
class HeaderSeq
{
public:
	typedef NamedPair<':', ';'> NamedAttribute;
public:
	HeaderSeq (LineSeq & lines)
		: _lines (lines), _done (lines.AtEnd ())
	{
		if (!AtEnd ())
		{
			_nextLine = _lines.Get ();
			Advance ();
		}
	}
	void Advance ();
	bool AtEnd () const 
	{
		if (_done)
		{
			Assert (_nextLine.empty () || _lines.AtEnd ());
		}
		return _done;
	}
	bool IsName (std::string const & name) const
	{
		return IsNocaseEqual (name, _namedAttribute.GetName ());
	}
	std::string const & GetValue () const { return _namedAttribute.GetValue (); }
	std::string const & GetName () const { return _namedAttribute.GetName (); }
	std::string const & GetComment () const { return _comment; }
private:
	LineSeq &		_lines;
	bool			_done;
	std::string		_nextLine;
	NamedAttribute	_namedAttribute;
	std::string		_comment;
};

#endif
