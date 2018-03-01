#if !defined (INPUTSOURCE_H)
#define INPUTSOURCE_H
//----------------------------------
// (c) Reliable Software 1998 - 2008
//----------------------------------

#include "SelectionMan.h"
#include "GlobalId.h"

#include <CmdLineScanner.h>
#include <auto_vector.h>
#include <StringOp.h>

// Commands before -GUI are executed in the background;
// after -GUI, in GUI mode.
// Project_Visit is special. It can occur only once.
// It's not added to any command list.

class CommandDescriptor: public NamedValues
{
public:
	CommandDescriptor (std::string cmdName)
		: _cmd (cmdName)
	{}

	void AddFileSelection (std::string file) { _fileSelection.insert (file); }
	void AddIdSelection (GlobalId id) { _idSelection.push_back (id); }
	void AddArgument (std::string const & argName, std::string const & argValue)
	{
		NamedValues::Add (argName, argValue);
	}
	std::string const & GetName () const { return _cmd; }
	// File selection iterator: files are lexicographically ordered
	string_iter BeginFiles () const 
	{
		return _fileSelection.begin ();
	}
	string_iter EndFiles () const 
	{
		return _fileSelection.end ();
	}
	// Id selection
	GidList const & GetIdSelection () const { return _idSelection; }

private:
	std::string				_cmd;
	std::set<std::string>	_fileSelection; // order them lexicographically for better performance
	GidList					_idSelection;
};

class InputSource
{
private:
	typedef auto_vector<CommandDescriptor> CmdList;
	typedef auto_vector<CommandDescriptor>::const_iterator CmdIter;

public:
	InputSource ();

	bool HasBackgroundCommands () const { return _bkgCommands.size () != 0; }
	bool HasGuiCommands () const { return _guiCommands.size () != 0; }

	bool StayInGui () const { return _stayInGui; }
	bool StartServer () const { return _startServer; }
	int  GetProjectId () const { return _projectId; }
	bool HasProjectId () const { return _projectId != -1; }
	NamedValues const & GetNamedArguments ();

	void SetProjectId (int id) { _projectId = id; }
	void SetStayInGui (bool doStay) { _stayInGui = doStay; }
	void SetStartServer (bool doStart) { _startServer = doStart; }

#if 0
	void Dump ();
#endif

	class Sequencer
	{
	public:
		Sequencer (CmdList const & cmds)
			: _cur (cmds.begin ()),
			  _end (cmds.end ())
		{}

		bool AtEnd () const { return _cur == _end; }
		void Advance () { ++_cur; }

		std::string const & GetCmdName () const { return (*_cur)->GetName (); }
		NamedValues const & GetArguments () const { return *(*_cur); }
		std::string const & GetCommand (SelectionManager & selectionMan) const;

	private:
		CmdIter	_cur;
		CmdIter	_end;
	};

	friend class Sequencer;
	friend class InputParser;

	Sequencer & GetCmdSeq () { return *_curSequencer; }

	void SwitchToGuiCommands ();
private:
	void OnEndInput ();

	bool						_stayInGui;
	bool						_startServer;
	int							_projectId;

	std::unique_ptr<Sequencer>	_curSequencer;
	CmdList						_bkgCommands;
	CmdList						_guiCommands;
};

class InputParser
{
public:
	InputParser (char const * str, InputSource & sink)
		: _sink (sink),
		  _scanner (str),
		  _curCommands (&_sink._bkgCommands)
	{}
	void Parse ();
private:
	void Command ();

	InputSource	&				_sink;
	CmdLineScanner				_scanner;
	InputSource::CmdList *		_curCommands;
};

//
// Command Line main procedure
//

int MainCmd (InputSource & inputSource, Win::Instance hInst);

#endif
