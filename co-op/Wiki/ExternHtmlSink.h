#if !defined (EXTERNHTMLSINK_H)
#define EXTERNHTMLSINK_H
//---------------------------
// (c) Reliable Software 2006
//---------------------------
#include "SqlHtmlSink.h"
#include <File/Path.h>

class ExternHtmlSink: public SqlHtmlSink
{
public:
	ExternHtmlSink (std::ostream & out, 
				FilePath const & toRootPath,
				FilePath const & curPath,
				FilePath const & sysPath,
				Sql::TuplesMap & tuplesMap)
		: SqlHtmlSink (out, curPath, sysPath, tuplesMap), _toRootPath (toRootPath), _useSystem (false)
	{}
	void InPlaceLink (std::string const & wikiUrl);
	void InternalLink (std::string const & wikiUrl, std::string const & wikiText);
	void ExternalLink (std::string const & wikiUrl, std::string const & wikiText);
	void InternalDeleteLink (std::string const & wikiUrl);
	void QueryField (std::string const & fieldName);
	bool UsesSystemLinks () const { return _useSystem; }
private:
	FilePath const & _toRootPath;
	bool _useSystem;
};

#endif
