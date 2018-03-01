//---------------------------
// (c) Reliable Software 2007
//---------------------------
import xml;
import WebAtlas;
import std.stdio;
import std.file;
import std.stream;
import std.path;
import std.string;

void main (string [] args)
{
	if (args.length < 3)
	{
		writeln ("expected xml file path");
		return;
	}
	string path = args [1];
	if (!exists (path) || isdir (path))
	{
		writefln ("Cannot find file: \"%s\"", path);
		return;
	}
	
	string targetPath = args [2];
	try
	{
		auto root = MakeTree (path);
		auto atlas = new WebSiteAtlas (root);

		foreach (file; atlas.LinksTo (tolower(targetPath)))
		{
			writeln (file);
		}

	}
	catch (Exception e)
	{
		writefln (e.toString);
	}
}


