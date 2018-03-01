#if !defined (SQLHTMLSINK_H)
#define SQLHTMLSINK_H
//---------------------------
// (c) Reliable Software 2006
//---------------------------
#include "precompiled.h"
#include "HtmlSink.h"
#include <File/Path.h>

class SqlHtmlSink: public HtmlSink
{
public:
	SqlHtmlSink (std::ostream & out, 
				FilePath const & curPath,
				FilePath const & sysPath,
				Sql::TuplesMap & tuplesMap)
		: HtmlSink (out, tuplesMap), _curPath (curPath), _sysPath (sysPath)
	{}
	void SqlCmd (Sql::Command const & cmd);
	void SqlSelectCmd (Sql::SelectCommand const & cmd);
	void SqlFromCmd (Sql::FromCommand const & cmd);
	void ListImages (bool isSystem);
protected:
	FilePath	_curPath;
	FilePath	_sysPath;
};

#endif
