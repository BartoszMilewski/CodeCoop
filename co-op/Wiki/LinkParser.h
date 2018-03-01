#if !defined (LINKPARSER_H)
#define LINKPARSER_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------
#include <File/Path.h>
#include <File/File.h>
#include <File/Dir.h>

void DecodeUrl (std::string & str);
void EncodeUrl (std::string & str);

// Wiki link has the form [[namespace:name#label]]

// With no namespace, it creates a link to the local file name.wiki
// With a non-special namespace, it creates a link to the file name.wiki in directory 'namespace'
// With special namespace, it creates a link to a non-text file in the directory 'namespace'
// For instance [[image:bird]] may link to image\bird.jpg or image\bird.gif, etc.,

// If the targer file exists, wiki link is converted to an appropriate pseydo-HTML link
// that might be further redirected when the user clicks on it.
// For instance, [[details]] becomes <a href="c:\project\details.wiki">details</a>
// Which, upon click, is redirected to c:\co-op\database\pojID\tmp.html, 
// where tmp.html was created on the fly from details.wiki
// [[image:bird]] is coverted to the image tag <img src="c:\project\image\bird.jpg"/>

// If the target file doesn't exist, a special suffix starting is added to the link
// For instance, [[details]] becomes href="c:\project\details.wiki\wikicreate"
// When the user clicks on the link, the file c:\project\details.wiki is created and added
// to co-op project, and the editor is fired
// [[image:bird]] becomes href="c:\project\image\bird\wikicreate"
// When the link is clicked, a "browse for file" dialog is open with wildcards *.jpg, *.gif, etc.
// When the user finds the file, it is copied to the image directory and added to project.
// (?) The user may also type in the URL of the image.
// The source page is reparsed, and, since the image file now exists, the link becomes
// <img src="path to image"/>

class LinkMaker
{
public:
	enum NameType { None, Image, Music };
private:
	static bool IsSpecial (NameType type)
	{
		return (type == Image || type == Music);
	}
	class Namespace
	{
	public:
		Namespace (std::vector<std::string> & nspaces);
		bool IsEmpty () const { return _nspace.empty (); }
		bool IsDelete () const { return _isDelete; };
		bool IsSystem () const { return _isSystem; }
		NameType GetType () const { return _type; }
	private:
		std::string _nspace;
		NameType	_type;
		bool		_isDelete;
		bool		_isSystem;
	};
public:
	LinkMaker (std::string const & wikiLink, 
				FilePath const & curPath, 
				FilePath const & sysPath);
	std::string GetWikiHref () const;
	std::string GetHtmlHref (FilePath const & toRoot) const;
	NameType GetType () const { return _type; }
	bool Exists () const { return _exists; }
	bool IsDelete () const { return _isDelete; }
	bool IsSystem () const { return _isSystem; }
private:
	void Parse (std::string const & wikiLink);
	virtual void CreateHref (FilePath const & curPath, 
							 FilePath const & sysPath);
private:
	typedef std::vector<std::string> Strings;

	Strings		_namespaces;
	FilePath	_path;
	std::string _name;
	std::string _label;
	NameType	_type;
	bool		_exists;
	bool		_isDelete;
	bool		_isSystem;
};

class LinkParser
{
public:
	static bool HasWikiExt (std::string const & url);
	LinkParser (std::string const & href);
	bool IsWiki () const
	{
		return _isCreate || _isDelete || _isWiki;
	}
	bool IsCreate () const
	{
		return _isCreate;
	}
	bool IsDelete () const
	{
		return _isDelete;
	}
	std::string const & GetPath () const { return _path; }
	std::string const & GetLabel () const { return _label; }
	std::string GetNamespace (std::string const & rootPath) const;
	std::string const & GetQueryString () const { return _query; }
private:
	std::string _path; // between double (triple) slash and '?'
	std::string	_query; // after '?'
	std::string _label;
	bool		_isWiki;
	bool		_isCreate;
	bool		_isDelete;
};

#endif
