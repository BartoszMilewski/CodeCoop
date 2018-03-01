//-----------------------------------
// (c) Reliable Software 1998 -- 2003
// ----------------------------------
#include "precompiled.h"
#include "Mailbox.h"
#include "ScriptName.h"
#include "Serialize.h"
#include "TransportHeader.h"
#include "TransportData.h"
#include "AlertMan.h"
#include "OutputSink.h"
#include <File/File.h>

Mailbox::Mailbox (FilePath const & path, DispatcherCmdExecutor & cmdExecutor)
	: _path (path), _scriptFiles (path), _cmdExecutor (cmdExecutor)
{
	_scriptPattern.push_back ("*.snc");
	_scriptPattern.push_back ("*.cnk");
	ExamineState ();
}

Mailbox::Mailbox (Mailbox const & src)
	: _path (src._path),
	  _status (src._status), 
	  _scriptFiles (src._path), 
	  _cmdExecutor (src._cmdExecutor),
	  _scriptPattern (src._scriptPattern)
{}

void Mailbox::DeleteScript (std::string const & scriptName)
{
	File::Delete (_path.GetFilePath (scriptName));
}

void Mailbox::DeleteScriptAllChunks (std::string const & scriptName)
{
	for (FileMultiSeq scriptSeq (_path, _scriptPattern); !scriptSeq.AtEnd (); scriptSeq.Advance ())
	{
		std::string physicalFile = scriptSeq.GetName ();
		if (ScriptFileName::AreEqualModuloChunkNumber (scriptName, physicalFile))
			File::Delete (_path.GetFilePath (physicalFile));
	}
}

void Mailbox::ChangePath (FilePath const & newPath)
{
	_path.Change (newPath);
	_scriptFiles.ChangePath (newPath);
	ExamineState ();
}

// Returns true if there are new scripts
bool Mailbox::UpdateFromDisk (NocaseSet & deletedScripts)
{
	bool foundNew = false;
	_scriptFiles.PrepareForRefresh ();
	for (FileMultiSeq scriptSeq (_path, _scriptPattern); !scriptSeq.AtEnd (); scriptSeq.Advance ())
	{
		if (_scriptFiles.Insert (scriptSeq.GetName ()))
			foundNew = true;
    }
	_scriptFiles.PruneDeletedScripts (deletedScripts);
	return foundNew;
}

void Mailbox::GetScriptFiles (std::vector<std::string> & filenames) const
{
	// don't clear result vector; append items at end;
	// this is a part of the contract
	filenames.reserve (_scriptFiles.size ());
	ScriptFileList::ConstSequencer seq (_scriptFiles);
	while (!seq.AtEnd ())
	{
		filenames.push_back (seq.GetName ());
		seq.Advance ();
	}
}

std::unique_ptr<TransportData> Mailbox::GetTransportData (std::string const & scriptFilename) const
{
	std::unique_ptr<TransportData> result;
	char const * fullPath = _path.GetFilePath (scriptFilename);
	if (File::Exists (fullPath))
	{
		try
		{
			FileDeserializer in (fullPath);
			std::unique_ptr<TransportHeader> th (new TransportHeader (in));
			result.reset (new TransportData (std::move(th)));
			ScriptSubHeader subHdr (in);
			result->SetComment (subHdr.GetComment ());
			ScriptStatus stat = _scriptFiles.GetStatus (scriptFilename);
			result->SetStatus (stat.LongDescription ());
		}
		catch (Win::Exception e)
		{
			TheOutput.Display (e);
		}
	}
	return result;
}

std::string Mailbox::GetScriptComment (std::string const & name) const
{
	char const * fullPath = _path.GetFilePath (name);
	if (File::Exists (fullPath))
	{
		try
		{
			FileDeserializer in (fullPath);
			ScriptSubHeader subHdr (in);
			return subHdr.GetComment ();
		}
		catch (...)
		{
			Win::ClearError ();
		}
	}
	return std::string ();
}

ScriptStatus Mailbox::GetScriptStatus (std::string const & name) const
{
	return _scriptFiles.GetStatus (name);
}

void Mailbox::ExamineState ()
{
	_status = stOk;
	if (!File::IsFolder (_path.GetDir ()))
	{
		_status = stInvalidPath;
	}
	else
	{
		// Revisit: check if folder is accessible
		//_status = stNotAccessible;
	}
}
