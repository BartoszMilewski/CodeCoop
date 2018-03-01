#if !defined (INPUTSOURCE_H)
#define INPUTSOURCE_H
//-----------------------------------
// (c) Reliable Software, 2004 - 2008
//-----------------------------------

#include <CmdLineScanner.h>

class NamedValues;

class CmdInputSource
{
public:
	class Sequencer;
	friend class Sequencer;

public:
	CmdInputSource (char const * str);
	bool IsEmpty () const { return _commands.size () == 0; }
	void SetRestart () { _isRestart = true; }
	bool IsRestart () { return _isRestart; }

	NamedValues const * GetNamedArguments () { return 0; }

private:
	void Command ();

private:
	CmdLineScanner				_scanner;
	std::vector<std::string>	_commands;
	bool						_isRestart;
};

class CmdInputSource::Sequencer
{
public:
	Sequencer (CmdInputSource const & inputSource)
		: _cur (inputSource._commands.begin ()),
		  _end (inputSource._commands.end ())
	{}

	bool AtEnd () const { return _cur == _end; }
	void Advance () { ++_cur; }

	std::string const & GetCommand () const { return *_cur; }

private:
	std::vector<std::string>::const_iterator	_cur;
	std::vector<std::string>::const_iterator	_end;
};

#endif
