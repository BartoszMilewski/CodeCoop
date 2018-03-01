//----------------------------------
//  (c) Reliable Software, 2008
//----------------------------------

module WebAtlas;

import xml;
import std.stdio;
import std.stream;
import std.path;
import std.string;
static import std.algorithm;

class WebSiteAtlas
{
public:
	this (Node root)
	{
		foreach (child; root)
		{
			string relPath;
			if (child.Name == "directory" && child.FindAttribute ("path", relPath))
			{
				ProcessFiles (child, tolower (relPath));
			}
		}
	}
	void ProcessFiles (Node root, string relPath)
	{
		assert (relPath == tolower (relPath));
		foreach (child; root)
		{
			string curFilePath;
			string type;
			if (child.Name == "file")
			{
				if (child.FindAttribute ("path", curFilePath))
					child.FindAttribute ("type", type);
				else
					throw new Exception ("file without path");
				curFilePath = tolower (curFilePath);
			}
			else
				throw new Exception ("non-file node");
			
			assert (curFilePath.length != 0);
			
			if (curFilePath in _targets)
			{
				_targets [curFilePath].Type = type;
			}
			else
			{
				_targets [curFilePath] = new TargetInfo (curFilePath, type);
			}
			
			switch (type)
			{
			case "html":
				_htmlFiles ~= curFilePath;
				break;
			case "img":
				_imgFiles ~= curFilePath;
				break;
			default:
				throw new Exception ("Illegal file type");
			}
			
			// Look for grandchildren that are anchors
			foreach (gChild; child)
			{
				if (gChild.Name == "a")
				{
					string href;
					if (!gChild.FindAttribute ("href", href))
						throw new Exception ("anchor without href");
					href = NormalizeHref (href, relPath);
					if (!(href in _targets))
						_targets [href] = new TargetInfo (href, "");

					_targets [href].AddLinkFrom (curFilePath);
				}
			}
			// Look for grandchildren that are image links
			foreach (gChild; child)
			{
				if (gChild.Name == "img")
				{
					string href;
					if (!gChild.FindAttribute ("src", href))
						throw new Exception ("image without source");
					href = NormalizeHref (href, relPath);
					if (!(href in _targets))
						_targets [href] = new TargetInfo (href, "");

					_targets [href].AddLinkFrom (curFilePath);
				}
			}
		}
	}

	bool HasFile (string path, string type)
	{
		if (type.length == 0)
			return false;

		if (type != "html" && type != "img")
		{
			string msg = "Invalid file type: " ~ type ~ "; path: " ~ path;
			throw new Exception (msg);
		}

		string [] fileList = (type == "html" ? _htmlFiles : _imgFiles);
		return std.algorithm.find (fileList, path) != std.algorithm.end (fileList);
	}
	
	string [] LinksTo (string targetPath)
	{
		const TargetInfo * tgt = (targetPath in _targets);
		if (tgt == null)
			throw new Exception ("Target not found");
		return tgt.LinksToMe;
	}

	string [] HtmlFiles () { return _htmlFiles; }
	string [] ImgFiles () { return _imgFiles; }
	TargetInfo [string] TargetInfoArr () { return _targets; }

private:
	TargetInfo [string] _targets;
	string [] _htmlFiles;
	string [] _imgFiles;
}

Node MakeTree (string path)
{
	// string s = `<root><child color="red">foo bar baz</child></root>`;
	// MemoryStream input = new MemoryStream (s.dup);
	File input = new File (path);
	if (!input.isOpen)
	{
		writefln ("Cannot open file for reading: ", path);
	}

	Tree tree = new Tree;
	TreeMaker treeMaker = new TreeMaker (tree);
	Scanner scanner = new Scanner (input);
	Parser parser = new Parser (scanner, treeMaker);
	parser.Parse ();

	return tree.Root ();
}

class TargetInfo
{
public:
	this (string path, string type)
	{
		_path = path;
		_type = type;
	}
	string Type ()
	{
		return _type;
	}
	string Type (string type)
	{
		_type = type;
		return _type;
	}
	string Path ()
	{
		return _path;
	}
	string Path (string path)
	{
		_path = path;
		return _path;
	}
	void AddLinkFrom (string src)
	{
		_linksToMe ~= src;
	}
	string [] LinksToMe () const { return _linksToMe; }
private:
	string _path;
	string _type;
	string [] _linksToMe;
}

bool IsExternalTarget (string href)
{
	if (href.length < 4)
		return false;

	string prefix = href [0..4];
	assert (prefix == tolower (prefix));
	return prefix == "http" || prefix == "mail" || prefix == "java";
}

string NormalizeHref (string href, string relPath)
{
	href = tolower (href);
	assert (relPath == tolower (relPath));
	if (IsExternalTarget (href))
		return href;

	string result;
	if (href.length >= 2 && href [0..2] == "..")
	{
		if (href.length == 2 || (href [2] != sep [0] && href [2] != altsep[0]))
		{
			writefln ("relPath=%s, href=%s", relPath, href);
			throw new Exception ("invalid relative path");
		}
			
		int pos = rfind (relPath, sep);
		if (pos == -1)
			result = href [3..$];
		else
			result = relPath [0..pos + 1] ~ href [3..$];
	}
	else
	{
		result = relPath ~ sep ~ href;
	}

	int pos = rfind (result, '#');
	if (pos != -1)
		result = result [0..pos];

	return replace (result, altsep, sep);
}

void DumpTree (Node node, int level=0)
{
	void Indent (int level)
	{
		for (int i = 0; i < level * 4; ++i)
			write (' ');
	}
	Indent (level);
	writefln (node.Name);
	foreach (a; node.Attributes)
	{
		Indent (level);
		writefln (" %s=%s", a.Name, a.Value);
	}
	foreach (n; node.Children)
	{
		DumpTree (n, level + 1);
	}
}

