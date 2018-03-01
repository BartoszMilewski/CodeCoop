import xml;
import WebAtlas;
import std.stdio;
import std.file;
import std.stream;
import std.path;
import std.string;

void main (string [] args)
{
	if (args.length < 2)
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
	
	try
	{
		auto root = MakeTree (path);
		auto atlas = new WebSiteAtlas (root);

		foreach (target; atlas.TargetInfoArr)
		{
			//writeln (target.Path);
			if (target.LinksToMe.length == 0)
			{
				// Orphan
				writefln ("    Orphan: %s", target.Path);
			}
			else if (!atlas.HasFile (target.Path, target.Type))
			{
				if (!IsExternalTarget (target.Path))
				{
					// Dead end
					writefln ("    Dead end: %s", target.Path);
				}
			}
			//foreach (toMe; target.LinksToMe)
			//	writefln ("    %s", toMe);
		}

/*			
		writeln ("==Files==");
		foreach (f; atlas.HtmlFiles)
			writeln (f);
		writeln ("==Images==");
		foreach (f; atlas.ImgFiles)
			writeln (f);
*/
	}
	catch (Exception e)
	{
		writefln (e.toString);
	}
}

