//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------
#include "precompiled.h"
#include "WikiConverter.h"
#include "LinkParser.h"
#include "Sql.h"
#include "SqlParser.h"
#include "GlobalFileNames.h"
#include "WikiDef.h"
#include <File/MemFile.h>

WikiPaths::WikiPaths (FilePath const & rootDir, FilePath const & systemDir)
	: _rootDir (rootDir), _systemDir (systemDir)
{
	_templPath = _rootDir.GetFilePath (WikiTemplate);
	if (!File::Exists (_templPath))
		_templPath = _systemDir.GetFilePath (WikiTemplate);
	_cssPath = _rootDir.GetFilePath (CssFileName);
	if (!File::Exists (_cssPath))
		_cssPath = _systemDir.GetFilePath (CssFileName);
}

//---------------------------------------
// Query String 
// Trailing part of URL starting with '?'
//---------------------------------------
QueryString::QueryString (std::string const & queryString)
{
	unsigned len = queryString.size ();
	if (len == 0 || queryString [0] != '?')
		return;
	unsigned begin = 1; // skip '?'
	while (begin != len)
	{
		unsigned sep = queryString.find ('&', begin);
		Property (queryString.substr (begin, sep - begin));
		begin = (sep == std::string::npos)? len: sep + 1;
	}
}

void QueryString::Property (std::string const & prop)
{
	unsigned eq = prop.find ('=');
	if (eq == std::string::npos)
		_fields [prop] = "";
	else
	{
		_fields [prop.substr (0, eq)] = Value (prop.substr (eq + 1));
	}
}

std::string QueryString::Value (std::string val)
{
	unsigned i = 0, j;
	while ((j = val.find ('+', i)) != std::string::npos)
	{
		val [j] = ' ';
		i = j + 1;
	}
	return val;
}

class ListMarker
{
public:
	ListMarker (WikiLexer & lex)
	{
		while (!lex.AtEnd () && lex.Get () == '*' || lex.Get () == '#')
		{
			_mark += lex.Get ();
			lex.Accept ();
		}
	}
	void Set (ListMarker const & src)
	{
		_mark = src._mark;
	}
	bool IsItem () const { return !_mark.empty (); }
	bool IsEqual (ListMarker const & marker)
	{
		return _mark == marker._mark;
	}
	char operator [] (unsigned i) const { return _mark.at (i); }
	unsigned size () const { return _mark.size (); }
	void clear () { _mark.clear (); }
	void resize (unsigned newSize) { _mark.resize (newSize); }
	bool HasPrefix (ListMarker const & prefix) const
	{
		return prefix.size () < _mark.size () 
			&& _mark.substr (0, prefix.size ()) == prefix._mark;
	}
private:
	std::string _mark;
};

//------------
// Line Parser
//------------
class WikiLineParser
{
public:
	WikiLineParser (WikiLexer & lex, HtmlSink & sink)
		: _lex (lex), _sink (sink)
	{}
	void Parse (bool isPre = false);
private:
	void WikiLink ();
	void QueryField ();
	void TableField ();
	void RegistryField ();
	void NoWiki ();
	void Link (bool isInternal);
	void InPlaceLink ();
	void Emph (unsigned count);
	bool IsTransport ();
private:
	WikiLexer & _lex;
	HtmlSink &  _sink;
	static char * Transports [];
};

char * WikiLineParser::Transports [] =
{
	"http:", "https:", "ftp:", "ftps:", "mailto:", "file:", "news:", "snews:", 0
};

bool WikiLineParser::IsTransport ()
{
	for (int i = 0; Transports [i] != 0; ++i)
		if (_lex.IsMatch (Transports [i]))
			return true;
	return false;
}

WikiConverter::WikiConverter (std::istream & in, HtmlSink & sink)
   : _parser (in, sink), _sink (sink)
{}

WikiConverter::~WikiConverter ()
{}

void WikiConverter::ParseAndSave (std::string const & templPath, std::string const & cssPath)
{
	if (!File::Exists (templPath))
	{
		throw Win::InternalException ("Missing wiki template file.\n(Run Code Co-op Setup again.)\n", templPath.c_str ());
	}
	MemFile templFile (templPath, File::ReadOnlyMode ());
	std::string templ (templFile.GetBuf (), templFile.GetBuf () + templFile.GetBufSize ());
	unsigned curPos = 0;
	unsigned cssPos = templ.find ("$CSS");
	if (cssPos != std::string::npos)
	{
		_sink.Copy (templ.begin () + curPos, templ.begin () + cssPos);
		_sink.Copy (cssPath.begin (), cssPath.end ());
		curPos = cssPos + strlen ("$CSS");
	}
	unsigned bodyPos = templ.find ("$BODY", curPos);
	if (bodyPos == std::string::npos)
		throw Win::InternalException ("Wiki template missing $BODY placeholder", templPath.c_str ());
	_sink.Copy (templ.begin () + curPos, templ.begin () + bodyPos);
	curPos = bodyPos + strlen ("$BODY");

	_parser.Parse ();
	_sink.Copy (templ.begin () + curPos, templ.end ());
}

//------------
// Wiki Parser
//------------
WikiParser::~WikiParser ()
{
	ClosePara ();
}

// Empty lines, Tables, and Blocks
void WikiParser::Parse ()
{
	while (!_lex.AtEnd ())
	{
		if (_lex.IsNewLine ())
		{
			if (!_inPara)
				_sink.Break ();
			ClosePara ();
			_lex.Accept ();
		}
		else if (_lex.Get () == '|')
		{
			ClosePara ();
			Table ();
		}
		else if (_lex.Get() == '\0')
			break;
		else
			Block ();
	}
}

void WikiParser::Table ()
{
	Assert (_lex.Get () == '|');
	_sink.BeginTable ();
	_lex.SetStopChar ('|');
	do
	{
		TableRow ();
	} while (!_lex.AtEnd () && _lex.Get () == '|');
	_lex.SetStopChar ();
	_sink.EndTable ();
}

void WikiParser::TableRow ()
{
	Assert (_lex.Get () == '|');
	_lex.Accept ();
	_sink.BeginTableRow ();
	do 
	{
		TableData (); // eats the ending '|'
	} while (!_lex.AtEnd () && _lex.Get () != '\n');
	if (!_lex.AtEnd ())
		_lex.Accept (); // newline
	_sink.EndTableRow ();
}

void WikiParser::TableData ()
{
	bool isHeader = false;
	if (_lex.Get () == '!')
	{
		isHeader = true;
		_lex.Accept ();
	}
	_sink.BeginTableData (isHeader);
	_lex.EatWhite ();
	do
	{
		Block ();
	} while (!_lex.AtEnd () && _lex.Get () != '|');
	if (!_lex.AtEnd ())
		_lex.Accept (); // vertical bar
	_sink.EndTableData (isHeader);
}

// Also used to parse table data (with stop char '|')
// Eats closing character
// Heading, List, Pre
void WikiParser::Block ()
{
	char c = _lex.Get ();
	if (c == '\n')
	{
		Assert (_lex.HasStopChar ());
		_lex.Accept ();
		_sink.Break ();
	}
	else if (c == '=')
	{
		ClosePara ();
		Heading ();
	}
	else if (c == '*' || c == '#')
	{
		ListMarker marker (_lex);
		List (marker);
	}
	else if (IsSpace (c))
	{
		Pre ();
	}
	else if (c == '-' && _lex.Count ('-') == 3)
	{
		ClosePara ();
		_sink.HorizontalRule ();
		_lex.Accept (3);
		Line ();
	}
	else if (c == '?')
	{
		ClosePara ();
		SqlCmd ();
	}
	else
	{
		if (!_inPara && !_lex.HasStopChar ())
		{
			_sink.BeginPara ();
			_inPara = true;
		}
		Line ();
	}
}

void WikiParser::Heading ()
{
	Assert (_lex.Get () == '=');
	unsigned count = _lex.Count ('=');
	if (count > 4)
		count = 4;
	_lex.Accept (count);
	_sink.BeginHeading (count);

	WikiLineParser line (_lex, _sink);
	line.Parse ();

	_sink.EndHeading (count);
}

// For instance, marker "*#*", level 0
// issue <ul>, call List with "*#*" level 1, which will
// issue <ol>, call list with "*#*" level 2, which will
// issue <ul>
// keep issuing items <li> as long as new markers equal original marker
// When deeper marker found, recurse deeper,
// When shorter marker found, close current list, return with shorter marker

void WikiParser::List (ListMarker & marker, unsigned level)
{
	Assert (level < 8);
	char mark = marker [level];
	_sink.BeginList (mark, level);
	for (;;)
	{
		ListMarker nextMarker (marker);
		if (marker.size () > level + 1 && level < 5)
		{
			marker.resize (level + 1);
			// Start a deeper list, it will set nextMarker
			List (nextMarker, level + 1);
		}
		else
		{
			_sink.BeginListItem (level);
			WikiLineParser line (_lex, _sink);
			line.Parse ();
			_sink.EndListItem ();
			char c = _lex.Get ();
			if (c == '*' || c == '#')
			{
				nextMarker = ListMarker (_lex);
			}
			else
			{
				marker.clear (); // no lookahead marker
				break;
			}
		}

		if (_lex.AtEnd ())
			break;

		if (marker.IsEqual (nextMarker))
			continue;

		if (nextMarker.HasPrefix (marker) && level < 5)
		{
			// deeper sublist
			List (nextMarker, level + 1);

			// nextMarker has been truncated
			if (nextMarker.IsEqual (marker))
				continue;
			else
			{
				marker.Set (nextMarker);
				break;
			}
		}

		if (marker.HasPrefix (nextMarker))
		{
			marker.Set (nextMarker);
			break;
		}
	}
	_sink.EndList (mark, level);
}

void WikiParser::Pre ()
{
	_sink.BeginPre ();
	do
	{
		WikiLineParser line (_lex, _sink);
		line.Parse (true); // isPre
		if (_lex.AtEnd () || _lex.IsStopChar ())
			break;
		char c = _lex.Get ();
	} while (IsSpace (_lex.Get ()));

	_sink.EndPre ();
}

void WikiParser::Line ()
{
	WikiLineParser line (_lex, _sink);
	line.Parse ();
}

void WikiParser::ClosePara ()
{
	if (_inPara)
	{
		_inPara = false;
		_sink.EndPara ();
	}
}

void WikiParser::SqlCmd ()
{
	Assert (_lex.Get () == '?');
	_lex.Accept ();
	if (!_useSql)
	{
		_sink.Text ("<b>Error: Recursive use of SQL</b>");
		return;
	}
	std::string line = _lex.GetLine ();
	Sql::CmdParser cmdParser (_lex, _sink.GetQueryString (), _sink.GetTuplesMap ());
	if (cmdParser.IsError ())
	{
		_sink.Text ("<br/><b>SQL syntax error in:</b><pre>");
		_sink.Text (line);
		_sink.Text ("</pre>before:<br/>");
	}
	else
	{
		_sqlCommand = cmdParser.ReleaseCommand ();
		_sink.SqlCmd (*_sqlCommand.get ());
	}
}

//-----------------
// Wiki Line Parser
//-----------------

// Consume final '\n' but not final stop char
void WikiLineParser::Parse (bool isPre)
{
	if (_lex.AtEnd ())
		return;
	char c = _lex.Get ();
	while (c != '\n' && !_lex.IsStopChar ())
	{
		switch (c)
		{
		case '[':
			WikiLink ();
			break;
		case '\'':
			{
				unsigned n = _lex.Count ('\'');
				if (n > 1)
					Emph (n);
				else
				{
					_sink.PutChar ('\'');
					_lex.Accept ();
				}
			}
			break;
		case '&':
			if (isPre)
				_sink.Amp ();
			else
				_sink.PutChar (c);
			_lex.Accept ();
			break;
		case '<':
			if (_lex.IsMatch ("<nowiki>"))
			{
				NoWiki ();
				break;
			}
			if (isPre)
			{
				_sink.LessThan ();
				_lex.Accept ();
				break;
			}
			// fall through
		default:
			_sink.PutChar (c);
			_lex.Accept ();
		}
		if (_lex.AtEnd ())
			break;
		c = _lex.Get ();
	}
	if (c == '\n')
	{
		_lex.Accept ();
		_sink.EndLine ();
	}
}

void WikiLineParser::Emph (unsigned count)
{
	if (count > 4)
		count = 4;
	_sink.Emph (count);
	_lex.Accept (count);
}

void WikiLineParser::NoWiki ()
{
	_lex.Accept (strlen ("<nowiki>"));
	while (!_lex.AtEnd ())
	{
		if (_lex.Get () == '<' &&_lex.IsMatch ("</nowiki>"))
		{
			_lex.Accept (strlen ("</nowiki>"));
			break;
		}
		_sink.PutChar (_lex.Get ());
		_lex.Accept ();
	}
}

void WikiLineParser::WikiLink ()
{
	Assert (_lex.Get () == '[');
	_lex.Accept ();
	if (_lex.AtEnd ())
	{
		_sink.PutChar ('[');
		return;
	}
	if (_lex.Get () == '?')
	{
		QueryField ();
	}
	else if (_lex.Get () == '!')
	{
		TableField ();
	}
	else if (_lex.Get () == '@')
	{
		RegistryField ();
	}
	else if (_lex.Get () == '[')
	{
		_lex.Accept ();
		if (_lex.AtEnd ())
		{
			_sink.Repeat ('[', 2);
			return;
		}
		if (_lex.Get () == '[')
		{
			_lex.Accept ();
			if (_lex.AtEnd ())
			{
				_sink.Repeat ('[', 3);
				return;
			}

			InPlaceLink ();
		}
		else
		{
			Link (true);
		}
	}
	else
		Link (false);
}

void WikiLineParser::QueryField ()
{
	Assert (_lex.Get () == '?');
	_lex.Accept ();
	std::string fieldName;
	char c = _lex.Get ();
	while (c != '\0' && c != ']' && c != '\n')
	{
		fieldName += c;
		_lex.Accept ();
		c = _lex.Get ();
	}
	if (c == '\0' || c == '\n')
	{
		_sink.Text ("[?");
		_sink.Text (fieldName);
		return;
	}
	Assert (c == ']');
	_lex.Accept ();
	_sink.QueryField (fieldName);
}

void WikiLineParser::TableField ()
{
	Assert (_lex.Get () == '!');
	_lex.Accept ();
	std::string fieldName;
	char c = _lex.Get ();
	while (c != '\0' && c != ']' && c != '\n')
	{
		fieldName += c;
		_lex.Accept ();
		c = _lex.Get ();
	}
	if (c == '\0' || c == '\n')
	{
		_sink.Text ("[!");
		_sink.Text (fieldName);
		return;
	}
	Assert (c == ']');
	_lex.Accept ();
	_sink.TableField (fieldName);
}

void WikiLineParser::RegistryField ()
{
	Assert (_lex.Get () == '@');
	_lex.Accept ();
	std::string fieldName;
	char c = _lex.Get ();
	while (c != '\0' && c != ']' && c != '\n')
	{
		fieldName += c;
		_lex.Accept ();
		c = _lex.Get ();
	}
	if (c == '\0' || c == '\n')
	{
		_sink.Text ("[!");
		_sink.Text (fieldName);
		return;
	}
	Assert (c == ']');
	_lex.Accept ();
	_sink.RegistryField (fieldName);
}

void WikiLineParser::InPlaceLink ()
{
	std::string wikiUrl;
	char c = _lex.Get ();
	while (c != ']' && c != '\n')
	{
		wikiUrl += c;
		_lex.Accept ();
		if (_lex.AtEnd ())
			return;
		c = _lex.Get ();
	}
	if (c == ']')
		_lex.Accept ();
	if (_lex.AtEnd ())
		return;
	if (_lex.Get () == ']')
		_lex.Accept ();
	if (_lex.AtEnd ())
		return;
	if (_lex.Get () == ']')
		_lex.Accept ();
	_sink.InPlaceLink (wikiUrl);
}

void WikiLineParser::Link (bool isInternal)
{
	std::string wikiUrl;
	// Don't treat array indexing etc. as links a [i]
	if (!isInternal && !IsTransport ())
	{
		_sink.PutChar ('[');
		return;
	}

	char c = _lex.Get ();
	do
	{
		wikiUrl += c;
		_lex.Accept ();
		if (_lex.AtEnd ())
			return;
		c = _lex.Get ();
	} while (c != '|' && c != ']' && c != '\n');
	std::string wikiText;
	if (c == '|')
	{
		_lex.Accept ();
		if (_lex.AtEnd ())
			return;
		c = _lex.Get ();
		while (c != ']' && c != '\n')
		{
			wikiText += c;
			_lex.Accept ();
			if (_lex.AtEnd ())
				return;
			c = _lex.Get ();
		}
	}
	if (c == ']')
		_lex.Accept ();
	if (isInternal)
	{
		if (!_lex.AtEnd () && _lex.Get () == ']')
			_lex.Accept ();
		_sink.InternalLink (wikiUrl, wikiText);
	}
	else
		_sink.ExternalLink (wikiUrl, wikiText);
}

//----------
// HTML Sink
//----------

void LocalHtmlSink::QueryField (std::string const & fieldName)
{
	_out << _queryString.GetField (fieldName);
}

void LocalHtmlSink::InPlaceLink (std::string const & wikiUrl)
{
	LinkMaker link (wikiUrl, _curPath, _sysPath);
	_out << "file:" << link.GetWikiHref ();
}

void LocalHtmlSink::InternalLink (std::string const & wikiUrl, std::string const & wikiText)
{
	LinkMaker link (wikiUrl, _curPath, _sysPath);
	if (link.GetType () == LinkMaker::Image && link.Exists () && !link.IsDelete ())
	{
		_out << "<img src=\"" << link.GetWikiHref () << "\"";
		if (!wikiText.empty ())
			_out << " alt=\"" << wikiText << "\"";
		_out << "/>";
	}
	else
	{
		_out << "<a href=\"";
		_out << link.GetWikiHref ();
		_out << "\"";
		if (!link.Exists ())
			_out << " class=\"create\"";
		_out << ">";
		if (!wikiText.empty ())
			_out << wikiText;
		else
			_out << wikiUrl;
		_out << "</a>";
	}
}

void LocalHtmlSink::ExternalLink (std::string const & wikiUrl, std::string const & wikiText)
{
	_out << "<a href=\"" << wikiUrl;
	_out << "\">";
	if (!wikiText.empty ())
		_out << wikiText;
	else
		_out << wikiUrl;
	_out << "</a>";
}

void LocalHtmlSink::InternalDeleteLink (std::string const & wikiUrl)
{
	std::string url = "Delete:";
	url += wikiUrl;
	LinkMaker link (url, _curPath, _sysPath);
	_out << "<a href=\"";
	_out << link.GetWikiHref ();
	_out << "\" class=\"delete\"";
	_out << ">";
	_out << wikiUrl;
	_out << "</a>";
}
