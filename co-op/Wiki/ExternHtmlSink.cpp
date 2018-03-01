//---------------------------
// (c) Reliable Software 2006
//---------------------------
#include "precompiled.h"
#include "ExternHtmlSink.h"
#include "LinkParser.h"
#include "Sql.h"
#include "WikiDef.h"

void ExternHtmlSink::QueryField (std::string const & fieldName)
{
	_out << fieldName;
}

void ExternHtmlSink::InPlaceLink (std::string const & wikiUrl)
{
	LinkMaker link (wikiUrl, _curPath, _sysPath);
	if (link.IsSystem ())
		_useSystem = true;
	_out << link.GetHtmlHref (_toRootPath);
}

void ExternHtmlSink::InternalLink (std::string const & wikiUrl, std::string const & wikiText)
{
	LinkMaker link (wikiUrl, _curPath, _sysPath);
	if (link.IsSystem ())
		_useSystem = true;
	if (link.GetType () == LinkMaker::Image && link.Exists () && !link.IsDelete ())
	{
		_out << "<img src=\"" << link.GetHtmlHref (_toRootPath) << "\"";
		if (!wikiText.empty ())
			_out << " alt=\"" << wikiText << "\"";
		_out << "/>";
	}
	else
	{
		_out << "<a href=\"";
		_out << link.GetHtmlHref (_toRootPath);
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

void ExternHtmlSink::ExternalLink (std::string const & wikiUrl, std::string const & wikiText)
{
	_out << "<a href=\"" << wikiUrl;
	_out << "\">";
	if (!wikiText.empty ())
		_out << wikiText;
	else
		_out << wikiUrl;
	_out << "</a>";
}

void ExternHtmlSink::InternalDeleteLink (std::string const & wikiUrl)
{
	LinkMaker link (wikiUrl, _curPath, _sysPath);
	if (link.IsSystem ())
		_useSystem = true;
	_out << "<a href=\"";
	_out << link.GetHtmlHref (_toRootPath);
	_out << "\" class=\"delete\"";
	_out << ">";
	_out << wikiUrl;
	_out << "</a>";
}
