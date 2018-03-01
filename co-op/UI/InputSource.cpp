//----------------------------------
// (c) Reliable Software 1998 - 2008
//----------------------------------

#include "precompiled.h"
#include "InputSource.h"
#include "SelectionMan.h"
#include "PathSequencer.h"

#include <File/Path.h>

#if 0
#include "OutputSink.h"
#include <StringOp.h>
#endif

//
// Input Source
// Parses command-line string and splits it into commands
// -command_name
// A command may be followed by a list of files
// -command_name file1 file2 file3
// It may also contain named arguments
// -command_name arg_name:arg_value ...
//
// For instance:
// -selection_checkin foo.cpp foo.h comment:"fixed bug #1234"
// -Project_Visit 3 -Selection_ChangeFileType type:"text file" "c:\projects\co-op50\co-op\AckBox.h"

InputSource::InputSource ()
	: _stayInGui (false),
	  _startServer (false),
	  _projectId (-1)
{
}

NamedValues const & InputSource::GetNamedArguments ()
{
	Assert (_curSequencer.get () != 0);
	return _curSequencer->GetArguments ();
}

void InputSource::OnEndInput ()
{
	_curSequencer.reset (new InputSource::Sequencer (_bkgCommands));
}

void InputSource::SwitchToGuiCommands ()
{
	_curSequencer.reset (new InputSource::Sequencer (_guiCommands));
}

void InputParser::Parse ()
{
	while (_scanner.Look () != CmdLineScanner::tokEnd)
	{
		Command ();
	}
	_sink.OnEndInput ();
}

void InputParser::Command ()
{
	// switch
	if (_scanner.Look () != CmdLineScanner::tokSwitch)
		throw Win::InternalException ("Command line error: "
									  "Command should start with a dash or a slash");
	_scanner.Accept ();
	// command_name
	if (_scanner.Look () != CmdLineScanner::tokString)
		throw Win::InternalException ("Command line error: "
									  "A dash or slash must be followed by command name");

	std::unique_ptr<CommandDescriptor> cmd (new CommandDescriptor (_scanner.GetString ()));
	_scanner.Accept ();
	// arguments
	while (_scanner.Look () != CmdLineScanner::tokSwitch 
		&& _scanner.Look () != CmdLineScanner::tokEnd)
	{
		if (_scanner.Look () != CmdLineScanner::tokString)
		{
			throw Win::InternalException ("Command line error: "
										  "Arguments to a command must be (quoted) strings");
		}
		std::string arg = _scanner.GetString ();
		_scanner.Accept ();
		if (_scanner.Look () == CmdLineScanner::tokColon)
		{
			// arg_name:arg_value pair
			_scanner.Accept ();
			if (_scanner.Look () != CmdLineScanner::tokString)
			{
				throw Win::InternalException ("Command line error: "
					"Named argument to a command must have a (quoted) string value");
			}
			std::string val = _scanner.GetString ();
			_scanner.Accept ();
			cmd->AddArgument (arg, val);
		}
		else
		{
			// Command selection
			if (FilePath::IsValid (arg) && FilePath::IsAbsolute (arg))
			{
				cmd->AddFileSelection (arg);
			}
			else if (arg.find_first_not_of ("0123456789abcdefABCDEF-") == std::string::npos)
			{
				if (arg.find_first_not_of ("0123456789") == std::string::npos)
				{
					// Decimal id
					cmd->AddIdSelection (ToInt (arg));
				}
				else
				{
					// Global id
					GlobalIdPack pack (arg);
					cmd->AddIdSelection (pack);
				}
			}
			else
			{
				throw Win::InternalException ("Command line error: Illegal command selection");
			}
		}
	}

	if (cmd->GetName () == "Project_Visit")
	{
		// Retrieve and store project id
		GidList const & ids = cmd->GetIdSelection ();
		int projectId = ids.front ();
		_sink.SetProjectId (projectId);
		// Don't push the command on the current vector
		return;
	}

	if (IsNocaseEqual (cmd->GetName (), "GUI"))
	{
		// This is not a real Code Co-op command just command line
		// switch telling us to stay open after executing commands
		_sink.SetStayInGui (true);
		// Switch command vectors
		_curCommands = &_sink._guiCommands;
		// Don't register "GUI" as command
		return;
	}

	if (IsNocaseEqual (cmd->GetName (), "NoGUI"))
	{
		// "NoGUI" really means: start server and listen to conversation
		_sink.SetStartServer (true);
		// The -NoGUI is treated as real Code Co-op command because
		// it contains named value -- 'conversation:<number>'
	}

	_curCommands->push_back (std::move(cmd));
}

class FileSelectionSequencer : public PathSequencer
{
public:
	FileSelectionSequencer (string_iter begin, string_iter end)
		: _cur (begin),
		  _end (end)
	{}

	bool AtEnd () const { return _cur == _end; }
	void Advance () { ++_cur; }

	char const * GetFilePath () const { return (*_cur).c_str (); }

private:
	string_iter	_cur;
	string_iter	_end;
};

#if 0
void InputSource::Dump ()
{
	std::string dump;
	if (_stayInGui)
		dump += "Keep Code Co-op open after executing commands\n";

	for (size_t i = 0; i < _commands.size (); i++)
	{
		CommandDescriptor const * cmd = _commands [i];
		dump += "Command name: ";
		dump += cmd->GetName ();
		dump += "\n";

		{
			string_iter it = cmd->BeginFiles ();
			string_iter end = cmd->EndFiles ();
			if (it != end)
			{
				dump += "    Files:\n";
				for (; it != end; ++it)
				{
					dump += "        ";
					dump += *it;
					dump += "\n";
				}
			}
		}
		{
			GidList const & ids = cmd->GetIdSelection ();
			if (!ids.empty ())
			{
				dump += "    Ids:\n";
				for (GidList::const_iterator it = ids.begin (); it != ids.end (); ++it)
				{
					dump += "        ";
					dump += GlobalIdPack (*it).ToString ();
					dump += "\n";
				}
			}
		}
		// cmd->Dump (dump);
	}
	TheOutput.Display (dump.c_str ());
}
#endif

std::string const & InputSource::Sequencer::GetCommand (SelectionManager & selectionMan) const
{
	Assert (!AtEnd ());
	selectionMan.Clear ();
	CommandDescriptor const * cmd = *_cur;
	// Add command selection to the selection manager
	FileSelectionSequencer fileSeq (cmd->BeginFiles (), cmd->EndFiles ());
	selectionMan.SetSelection (fileSeq);
	selectionMan.SetSelection (cmd->GetIdSelection ());
	return cmd->GetName ();
}
