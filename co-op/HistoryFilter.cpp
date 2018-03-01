//------------------------------------
//  (c) Reliable Software, 2002 - 2008
//------------------------------------

#include "precompiled.h"
#include "HistoryFilter.h"
#include "ScriptCommands.h"
#include "Workspace.h"

void History::Filter::AddCommand (std::unique_ptr<FileCmd> cmd, GlobalId scriptId)
{
	_selection.AddCommand (std::move(cmd));
}

bool History::RangeAllFiles::IsScriptRelevant (GlobalId scriptId)
{
	if (_startVersionSeen)
	{
		_atEnd = (scriptId == _idVersionStop);
		if (_atEnd)
			return false;
		else
			return true;
	}
	else if (scriptId == _idVersionStart)
	{
		_startVersionSeen = true;
		return true;
	}
	else
	{
		Assert (scriptId != _idVersionStop);
		Assert (!_atEnd);
	}
	return false;
}

void History::RepairFilter::AddCommand (std::unique_ptr<FileCmd> cmd, GlobalId scriptId)
{
	if (cmd->GetType () == typeWholeFile)
		_preSelectedGids.erase (cmd->GetGlobalId ());
	_selection.AddCommand (std::move(cmd));
}

// Move leftover files from _preSelectedGids to unrecoverable set
void History::RepairFilter::MoveUnrecoverableFiles (GidSet & unrecoverable)
{
	unrecoverable.insert (_preSelectedGids.begin (), _preSelectedGids.end ());
	if (!unrecoverable.empty())
		_selection.Erase(unrecoverable);
}

void History::BlameFilter::AddCommand (std::unique_ptr<FileCmd> cmd, GlobalId scriptId)
{
	_atEnd = (cmd->GetType () == typeWholeFile);
	_selection.AddCommand (std::move(cmd));
	_scriptIds.push_back (scriptId);
}

char const History::FilterScanner::_whiteChars [] = " ";
char const History::FilterScanner::_stopChars [] = " ";

std::string History::FilterScanner::GetTokenString ()
{
	return _input.substr (_curTokenStringStart, _curTokenStringEnd - _curTokenStringStart);
}

void History::FilterScanner::Accept ()
{
	if (!EatWhite ())
	{
		_curToken = tokenEnd;
		return;
	}

	if (_input [_curPosition] == GlobalIdPack::GetLeftBracket ()) // '<'
	{
		_curToken = tokenFileId;
		_curPosition++;
		_curTokenStringStart = _curPosition;
		_curTokenStringEnd = _input.find (GlobalIdPack::GetRightBracket (), _curPosition);
		if (_curTokenStringEnd == std::string::npos)
		{
			_curTokenStringEnd = _input.length ();
			_curPosition = _curTokenStringEnd;
		}
		else
		{
			_curPosition = _curTokenStringEnd + 1; // eat closing bracket
		}
	}
	else if (_input [_curPosition] == GlobalIdPack::GetRightBracket ()) // '>'
	{
		_curToken = tokenScriptId;
		_curPosition++;
		_curTokenStringStart = _curPosition;
		_curTokenStringEnd = _input.length ();
		_curPosition = _curTokenStringEnd;
	}
	else
	{
		_curToken = tokenName;
		AcceptTokenString ();
		if (_input.compare (_curTokenStringStart, 3, "*.*") == 0)
			_curToken = tokenStarDotStar;
	}
}

bool History::FilterScanner::EatWhite ()
{
	_curPosition = _input.find_first_not_of (_whiteChars, _curPosition);
	return _curPosition != std::string::npos;
}

void History::FilterScanner::AcceptTokenString ()
{
	// Remove "" if present
	if (_input [_curPosition] == '"')
	{
		_curPosition++;
		_curTokenStringStart = _curPosition;
		_curTokenStringEnd = _input.find ('"', _curPosition);
		if (_curTokenStringEnd == std::string::npos)
		{
			_curTokenStringEnd = _input.length ();
			_curPosition = _curTokenStringEnd;
		}
		else
		{
			_curPosition = _curTokenStringEnd + 1; // eat closing quote
		}
	}
	else
	{
		_curTokenStringStart = _curPosition;
		_curTokenStringEnd = _input.find_first_of (_stopChars, _curPosition);
		if (_curTokenStringEnd == std::string::npos)
			_curTokenStringEnd = _input.length ();
		_curPosition = _curTokenStringEnd;
	}
}
