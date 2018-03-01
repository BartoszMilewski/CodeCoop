//
// (c) Reliable Software 1997 -- 2002
//

#include "precompiled.h"
#include "CommandParams.h"
#include "IpcExchange.h"
#include <StringOp.h>
#include <Xml/Scanner.h>
#include <Xml/XmlParser.h>

#include <locale>
#include <sstream>

using namespace CmdLine;

SwitchMap SwitchMap::_switchChars [LastSwitch] =
{
	{ AfterFile,		'a'},
	{ BeforeFile,		'b'},
    { Comment,			'c'},
	{ Status,			'f'},
	{ ShortInfo,		'i'},
	{ EditLog,			'l'},
	{ Mode,				'm'},
    { ProjectFile,		'p'},
    { ReferenceFile,	'r'},
    { Title,			't'},
	{ ShowWhat,			'w'},
    { XmlHandle,		'x'}
};

void CommandParams::Init (char const * args)
{
    _fileCount = 0;
    StringParser parser (args);

    while (!parser.AtEnd ())
    {
        parser.EatWhite ();
        if (parser.AtEnd ())
            break;
		SwitchToken token;
        if (parser.GetChar () == '/' || parser.GetChar () == '-')
        {
			int switchSign = parser.GetChar ();
            // it's a command line switch
            parser.SkipChar ();
            int switchChar = parser.GetChar ();
            token = SwitchMap::CharToToken (switchChar);
            if (token == LastSwitch)
            {
				std::string switchStr ("\"  \"");
				switchStr [1] = switchSign;
				switchStr [2] = switchChar;
                throw Win::InternalException ("Invalid command line switch", switchStr.c_str ());
            }
			if (token == XmlHandle)
			{
				// Handle to the shared memory buffer containig command data
				parser.EatWord ();
				parser.EatWhite ();
				unsigned int handle;
				if (parser.GetChar () == '0')
				{
					if (parser.GetHexNumber (handle))
					{
						XmlBuf buf (handle);
						std::istream in (&buf);
						XML::Scanner scanner (in);
						XML::TreeMaker treeMaker (*_xmlArgs);
						XML::Parser parser (scanner, treeMaker);
						parser.Parse ();
						return;
					}
				}
				else
				{
		            parser.MarkBeginning ();
				    parser.FindWhite ();
					std::string path (parser.GetBeginning (), parser.GetCurLen ());
					if (File::Exists (path))
					{
						std::ifstream in (path.c_str ());
						XML::Scanner scanner (in);
						XML::TreeMaker treeMaker (*_xmlArgs);
						XML::Parser parser (scanner, treeMaker);
						parser.Parse ();
						return;
					}
					else
						throw Win::InternalException ("Argument file doesn't exist", path.c_str ());
				}
			}
			else
			{
				// Regular command line switch
				parser.SkipChar ();
				if (parser.GetChar () == ':')
					parser.SkipChar ();
			}
        }
        else
        {
            // default
            if (_fileCount == 0)
				token = ProjectFile;
            else if (_fileCount == 1)
                token = BeforeFile;
            else
                throw Win::InternalException ("Bad command line", args);
            _fileCount++;
        }

        if (parser.GetChar () == '\"')
        {
            parser.SkipChar ();
            parser.MarkBeginning ();
            if (parser.FindQuote ())
            {
                std::string str (parser.GetBeginning (), parser.GetCurLen ());
				AddFile (str, token);
                parser.SkipChar ();  // closing "
            }
        }
        else
        {
            parser.MarkBeginning ();
            parser.FindWhite ();
            std::string str (parser.GetBeginning (), parser.GetCurLen ());
			AddFile (str, token);
        }
    }
}

void CommandParams::AddFile (std::string const & path, SwitchToken token)
{
	XML::Node * root = _xmlArgs->GetRootEdit ();
	if (root == 0)
	{
		root = _xmlArgs->SetRoot (token == ProjectFile? "edit": "diff");
	}
	Assert (root != 0);
	if (token != ProjectFile)
		root->SetName ("diff");
	char const * role = (token == ProjectFile)? "current": "before";
	XML::Node * child = root->FindEditChildByAttrib ("file", "role", role);
	if (child == 0)
	{
		child = root->AddEmptyChild ("file");
		child->AddAttribute ("role", role);
	}
	else
	{
		child->RemoveAttribute ("path");
	}
	child->AddAttribute ("path", path);
}


char const * CommandParams::GetString (SwitchToken tok) const 
{
	TokenStringMap::const_iterator it = _strings.find (tok);
	return (it != _strings.end ())? it->second.c_str (): "";
}

void CommandParams::Clear ()
{
	_fileCount = 0;
	_strings.clear ();
}

SwitchToken SwitchMap::CharToToken (int c)
{
    int clow = ::ToLower (c);
    for (int i = 0; i < LastSwitch; i++)
    {
        if (_switchChars [i]._char == clow)
            return _switchChars [i]._token;
    }
    return LastSwitch;
}

int SwitchMap::TokenToChar (SwitchToken tok)
{
    for (int i = 0; i < LastSwitch; i++)
    {
        if (_switchChars [i]._token == tok)
            return _switchChars [i]._char;
    }
    return 0;
}

void StringParser::EatWord ()
{
    while (::IsAlpha (_string [_curOffset]))
        _curOffset++;
}

void StringParser::EatWhite ()
{
    while (::IsSpace (_string [_curOffset]))
        _curOffset++;
}

void StringParser::FindWhite ()
{
    while (!AtEnd () && !::IsSpace (_string [_curOffset]))
        _curOffset++;
}

bool StringParser::FindQuote ()
{
    while (!AtEnd ())
    {
        if (_string [_curOffset] == '\"')
            return true;
        _curOffset++;
    }
    return false;
}

bool StringParser::GetHexNumber (unsigned int & number)
{
	EatWhite ();
	char c = GetChar ();
	if (std::isxdigit (c, std::locale ("C")))
	{
		MarkBeginning ();
		std::istringstream in (GetBeginning ());
		in >> std::hex >> number;
		FindWhite ();
		return true;
	}
	return false;
}

bool CommandParams::IsSet (SwitchToken tok, char const * value) const
{
	std::string showStr (GetString (tok));
	return !showStr.empty () && IsNocaseEqual (showStr, value);
}