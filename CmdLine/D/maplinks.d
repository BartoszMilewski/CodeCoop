//---------------------------
// (c) Reliable Software 2007
//---------------------------
import std.stdio;
import std.string;
import std.file;
import std.regexp;
import std.path;

void ProcessFiles ( string relPath, 
					bool delegate (string) onFilePath, 
					void delegate (string, string) onFileText)
{
	string [] dirStack;

	writefln (`<directory path="%s">`, relPath);
	bool callback (DirEntry * de)
	{
		if (de.isfile)
		{
			if (onFilePath (de.name))
			{
				string s = cast (string) read (de.name);
				onFileText (de.name, s);
			}
		}
		else if (de.isdir)
		{
			dirStack ~= de.name;
		}
		return true;
	}
	
	listdir (relPath, &callback);
	writeln (`</directory>`);

	foreach (dir; dirStack)
		ProcessFiles (dir, onFilePath, onFileText);

}

string quote (string str)
{
	if (str.length == 0) 
		return `""`;
	if (str [0] == '"')
	{
		if (str.length <= 1 || str [$-1] != '"')
		{
			throw new Exception ("Missing closing quote");
		}
		return encode (str);
	}
	assert (str [$-1] != '"');
	string result = `"`;
	return encode (result ~ str ~ `"`);
}

string encode (string quotedString)
{
	// For now, just strip the query string
	int begin = find (quotedString, '?');
	if (begin != -1)
		return quotedString [0..begin] ~ `"`;
	return quotedString;
}

void main (string [] args)
{
	if (args.length < 2)
	{
		writeln ("expected directory path to scan");
		return;
	}
	string rootPath = args [1];
	if (!exists (rootPath) || !isdir (rootPath))
	{
		writefln ("Not a valid directory path: \"%s\"", rootPath);
		return;
	}
	chdir (rootPath);

	RegExp anchorPat = new RegExp (`<A\s+HREF\s*=\s*(".+?".*?|.+?).*>((.|\n)+?)</A>`, "i");
	RegExp imgLinkPat = new RegExp (`<IMG\s+SRC\s*=\s*(".+?".*?|.+?).*/?>`, "i");
	RegExp aPat = new RegExp (`<a .*?href`, "i");
	RegExp iPat = new RegExp (`<img `, "i");
	RegExp htmlPat = new RegExp ("html?$", "i");
	RegExp imgPat = new RegExp ("gif$", "i");
	string titlePat = `<TITLE>((.|\n)*?)</TITLE>`;

	bool isHtmlFile (string path)
	{
		string ext = getExt (path);
		if (htmlPat.test (ext) != 0)
			return true;
		if (imgPat.test (ext) != 0)
		{
			writefln (`<file path=%s type="img"></file>`, quote(path));
		}
		return false;
	}
	
	void onHtmlFileText (string path, string text)
	{
		writefln (`<file path=%s type="html">`, quote(path));
		auto mch = search (text, titlePat, "i");
		if (mch !is null)
		{
			writefln ("<title>%s</title>", mch[1]);
		}

		int anchorCount = 0;
		foreach (m; anchorPat.search (text))
		{
			anchorCount++;
			string href = quote (m[1]);
			string anchor = m[2];
			writefln (`    <a href=%s>%s</a>`, href, anchor);
		}
		int imageCount = 0;
		foreach (m; imgLinkPat.search (text))
		{
			imageCount++;
			string src = quote (m[1]);
			writefln (`    <img src=%s/>`, src);
		}

		int i = 0;
		foreach (m; aPat.search (text))
		{
			i++;
		}
		if (anchorCount != i)
		{
			string msg = "Un-matched links in file " ~ path;
			msg = msg ~ "; anchor count = " ~ toString (anchorCount);
			msg = msg ~ "; i = " ~ toString (i);
			throw new Exception (msg);
		}

		i = 0;
		foreach (m; iPat.search (text))
		{
			i++;
		}

		if (imageCount != i)
		{
			string msg = "Un-matched image links in file " ~ path;
			msg = msg ~ "; image count = " ~ toString (imageCount);
			msg = msg ~ "; i = " ~ toString (i);
			throw new Exception (msg);
		}
		writeln (`</file>`);
	}
	
	writefln (`<root path="%s">`, rootPath);
	ProcessFiles ("", &isHtmlFile, &onHtmlFileText);
	writeln ("</root>");
}

unittest
{
	assert (quote ("") == `""`);
	assert (quote ("a") == `"a"`);
	assert (quote (`"a"`) == `"a"`);
}
