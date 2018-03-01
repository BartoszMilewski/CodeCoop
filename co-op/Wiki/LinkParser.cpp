//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------
#include "precompiled.h"
#include "LinkParser.h"
#include "WikiDef.h"

bool LinkParser::HasWikiExt (std::string const & url)
{
	return url.size () > 5 && IsNocaseEqual (url.substr (url.size () - 5), ".wiki");
}

void DecodeUrl (std::string & str)
{
	unsigned off = 0;
	for (;;)
	{
		off = str.find ('%', off);
		if (off == std::string::npos)
			break;
		std::string numStr = str.substr (off + 1, 2);
		char * endPtr;
		long code = std::strtol (numStr.c_str (), &endPtr, 16);
		if (endPtr - numStr.c_str () == 2)
		{
			str [off] = static_cast<char> (code);
			++off;
			str.erase (off, 2);
		}
		else
			++off;
	}
}

void EncodeUrl (std::string & str)
{
	// replace spaces with %20
	for (;;) 
	{
		unsigned i = str.find (' ');
		if (i == std::string::npos)
			break;
		str.erase (i, 1);
		str.insert (i, "%20");
	}
}

LinkMaker::Namespace::Namespace (std::vector<std::string> & nspaces)
	: _type (None), _isDelete (false), _isSystem (false)
{
	if (nspaces.size () > 0)
	{
		if (nspaces.front () == Wiki::SYSTEM)
		{
			_isSystem = true;
			nspaces.erase (nspaces.begin ());
			if (nspaces.size () == 0)
				return;
		}
		if (IsNocaseEqual (nspaces.front (), "delete"))
		{
			_isDelete = true;
			nspaces.erase (nspaces.begin ());
			if (nspaces.size () == 0)
				return;
		}
		_nspace = nspaces.front ();
		if (IsNocaseEqual (_nspace, "image"))
			_type = Image;
	}
}

LinkMaker::LinkMaker (std::string const & wikiLink, 
					  FilePath const & curPath,
					  FilePath const & sysPath)
	: _type (None), _exists (false), _isDelete (false), _isSystem (false)
{
	Parse (wikiLink);
	CreateHref (curPath, sysPath);
}

void LinkMaker::Parse (std::string const & wikiLink)
{
	unsigned start = 0;
	for (;;)
	{
		unsigned end = wikiLink.find (':', start);
		if (end == std::string::npos)
		{
			end = wikiLink.find ('#', start);
			_name = wikiLink.substr (start, end);
			if (end != std::string::npos)
				_label = wikiLink.substr (end);
			break;
		}
		if (end != start)
			_namespaces.push_back (wikiLink.substr (start, end - start));
		start = end + 1;
	}
}

void LinkMaker::CreateHref (FilePath const & curPath,
							FilePath const & sysPath)
{
	Namespace nspace (_namespaces);
	_type = nspace.GetType ();
	std::string fileName (_name);
	if (!fileName.empty () && !IsSpecial (_type))
		fileName += ".wiki";

	if (nspace.IsSystem ())
	{
		_isSystem = true;
		_path = sysPath;
	}
	else
		_path = curPath;

	for (Strings::const_iterator it = _namespaces.begin (); it != _namespaces.end (); ++it)
		_path.DirDown (it->c_str ());
	_exists = File::Exists (_path.GetFilePath (fileName));

	if (!_exists && IsSpecial (_type) && File::Exists (_path.GetDir ()))
	{
		// try extensions
		std::string pattern (_path.GetFilePath (fileName));
		pattern += ".*";
		FileSeq seq (pattern);
		if (!seq.AtEnd ())
		{
			_name = seq.GetName ();
			_exists = true;
		}
	}
	if (nspace.IsDelete ())
		_isDelete = true;
}

std::string LinkMaker::GetWikiHref () const
{
	if (_name.empty ())
		return _label;

	std::string href = _path.GetFilePath (_name);
	if (!IsSpecial (_type))
		href += ".wiki";

	href += _label;

	if (_isDelete)
		href += "\\WIKIDELETE";
	else if (!_exists)
		href += "\\WIKICREATE";
	return href;
}

// General link is of the form ../../namsp1/namesp2/file.ext
// The dotted path is toRoot. Notice forward slashes!
std::string LinkMaker::GetHtmlHref (FilePath const & toRoot) const
{
	if (_name.empty ())
	{
		return _label;
	}

	FilePath path (toRoot);
	if (IsSystem ())
		path.DirDown ("system", true); // use slash
	for (Strings::const_iterator it = _namespaces.begin (); it != _namespaces.end (); ++it)
		path.DirDown (it->c_str (), true); // use slash
	std::string href = path.GetFilePath (_name.c_str (), true);
	if (!IsSpecial (_type))
	{
		href += ".html";
		href += _label;
	}
	return href;
}

// Example: file:///c:\project\file.wiki#label/WIKICREATE
LinkParser::LinkParser (std::string const & href)
	: _isWiki (false), _isCreate (false), _isDelete (false)
{
	unsigned delim = href.rfind ('?');
	if (delim != std::string::npos)
	{
		_query = href.substr (delim, href.size () - delim);
		DecodeUrl (_query);
	}
	// parse path
	unsigned pathStart = href.find ("//");
	if (pathStart != std::string::npos)
	{
		pathStart += 2;
		if (pathStart < href.size () && href [pathStart] == '/')
			++pathStart; // eat the third slash
	}
	else
		pathStart = 0;

	unsigned end = delim;
	if (end == std::string::npos)
	{
		end = href.size ();
		if (end > 0 && href [end - 1] == '/')
			--end;
	}

	// Example: c:\project\file.wiki#label/WIKICREATE
	_path = href.substr (pathStart, end - pathStart);
	std::string lastSegment = _path;

	// parse last segment for "WIKICREATE"
	unsigned lastSlash = _path.find_last_of ("/\\");
	if (lastSlash != std::string::npos)
	{
		lastSegment.assign (_path.substr (lastSlash + 1));
		if (lastSegment == "WIKICREATE")
		{
			_isCreate = true;
			_path.erase (lastSlash);
		}
		else if (lastSegment == "WIKIDELETE")
		{
			_isDelete = true;
			_path.erase (lastSlash);
		}
	}
	// Example: c:\project\file.wiki#label

	// parse the label
	unsigned labelPos = _path.rfind ('#');
	if (labelPos != std::string::npos)
	{
		_label.assign (_path.substr (labelPos));
		_path.erase (labelPos);
	}

	if (!_isCreate && !_isDelete)
	{
		_isWiki = HasWikiExt (_path);
	}

	DecodeUrl (_path);
}

std::string LinkParser::GetNamespace (std::string const & rootPath) const
{
	if (_path.size () > rootPath.size () + 1 
		&& IsNocaseEqual (rootPath, _path.substr (0, rootPath.size ())))
	{
		unsigned lastSlash = _path.rfind ('\\');
		if (lastSlash == std::string::npos)
			lastSlash = _path.size ();
		return _path.substr (rootPath.size () + 1, lastSlash - rootPath.size () - 1);
	}
	return std::string ();
}

