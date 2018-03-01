#if !defined (WIKICONVERTER_H)
#define WIKICONVERTER_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------
#include "WikiLexer.h"
#include "SqlHtmlSink.h"
#include <File/Path.h>
#include <StringOp.h>
#include <iostream>

class WikiPaths
{
public:
	WikiPaths (FilePath const & rootDir, FilePath const & systemDir);
	std::string const & GetCssPath () const { return _cssPath; }
	std::string const & GetTemplPath () const { return _templPath; }
private:
	FilePath _rootDir;
	FilePath _systemDir;
	std::string _templPath;
	std::string _cssPath;
};

class QueryString
{
public:
	QueryString (std::string const & queryString);
	std::string const & GetField (std::string const & fieldName) const
	{
		NocaseMap<std::string>::const_iterator it = _fields.find (fieldName);
		if (it == _fields.end ())
			return _empty;
		return it->second;
	}
private:
	void Property (std::string const & prop);
	std::string Value (std::string val);
private:
	NocaseMap<std::string> _fields;
	std::string _empty;
};

class LocalHtmlSink: public SqlHtmlSink
{
public:
	LocalHtmlSink (std::ostream & out, 
				FilePath const & curPath,
				FilePath const & sysPath,
				std::string const & queryString,
				Sql::TuplesMap & tuplesMap)
		: SqlHtmlSink (out, curPath, sysPath, tuplesMap), 
		  _curPath (curPath), 
		  _sysPath (sysPath), 
		  _queryString (queryString)
	{}
	void InPlaceLink (std::string const & wikiUrl);
	void InternalLink (std::string const & wikiUrl, std::string const & wikiText);
	void ExternalLink (std::string const & wikiUrl, std::string const & wikiText);
	void InternalDeleteLink (std::string const & wikiUrl);
	QueryString	const * GetQueryString () const { return &_queryString; }
	void QueryField (std::string const & fieldName);
private:
	FilePath	_curPath;
	FilePath	_sysPath;
	QueryString	_queryString;
};

class ListMarker;

class WikiParser
{
public:
	WikiParser (std::istream & in, HtmlSink & sink, bool useSql = true)
		:_lex (in), _sink (sink), _useSql (useSql), _inPara (false)
	{}
	~WikiParser ();
	void Parse ();
	Sql::Command * GetSqlCommand () { return _sqlCommand.get (); }
private:
	// Productions
	void Table ();
	void TableRow ();
	void TableData ();
	void Block ();
	void Heading ();
	void Pre ();
	void List (ListMarker & marker, unsigned level = 0);
	void SqlCmd ();
	void Line ();

	void ClosePara ();
private:
	bool		_useSql;
	WikiLexer	_lex;
	HtmlSink &	_sink;
	std::unique_ptr<Sql::Command> _sqlCommand;
	// State
	bool		_inPara;
};

class WikiConverter
{
public:
	WikiConverter (std::istream & in, HtmlSink & sink);
	~WikiConverter ();
	void ParseAndSave (std::string const & templPath, std::string const & cssPath);
	Sql::Command * GetSqlCommand () { return _parser.GetSqlCommand (); }
private:
	HtmlSink &	_sink;
	WikiParser	_parser;
};

#endif
