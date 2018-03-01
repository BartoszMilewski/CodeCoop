#if !defined (HTMLSINK_H)
#define HTMLSINK_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------
#include <ostream>
#include <Sql.h>
class QueryString;
class FilePath;

class HtmlSink
{
public:
	HtmlSink (std::ostream & out, Sql::TuplesMap & tuplesMap)
		: _out (out), _tuples (tuplesMap)
	{}
	virtual ~HtmlSink ();
	void Copy (std::string::const_iterator begin, std::string::const_iterator end);
	void PutChar (char c)
	{
		_out << c;
	}
	void Repeat (char c, unsigned count)
	{
		while (count-- != 0)
			_out << c;
	}
	void BeginPara ()
	{
		_out << "<p>";
	}
	void EndPara ()
	{
		_out << "</p>";
	}
	void Break ()
	{
		_out << "<br/>";
	}
	void EndLine ()
	{
		_out << std::endl;
	}
	void Text (std::string const & text)
	{
		_out << text;
	}
	void BeginPre ()
	{
		_out << "<pre>";
	}
	void EndPre ()
	{
		_out << "</pre>" << std::endl;
	}
	void BeginList (char marker, unsigned level)
	{
		Repeat (' ', 2 * level);
		Assert (marker == '*' || marker == '#');
		if (marker == '*')
			_out << "<ul>";
		else
			_out << "<ol>";
		_out << std::endl;
	}
	void EndList (char marker, unsigned level)
	{
		Repeat (' ', 2 * level);
		Assert (marker == '*' || marker == '#');
		if (marker == '*')
			_out << "</ul>";
		else
			_out << "</ol>";
		_out << std::endl;
	}
	void BeginTable ()
	{
		_out << "<table>" << std::endl;
	}
	void EndTable ()
	{
		_out << "</table>" << std::endl;
	}
	void BeginTableRow ()
	{
		_out << "  <tr>" << std::endl;
	}
	void EndTableRow ()
	{
		_out << "  </tr>" << std::endl;
	}
	void BeginTableData (bool isHeader)
	{
		if (isHeader)
			_out << "    <th>";
		else
			_out << "    <td>";
	}
	void EndTableData (bool isHeader)
	{
		if (isHeader)
			_out << "  </th>" << std::endl;
		else
			_out << "  </td>" << std::endl;
	}
	void BeginListItem (unsigned level)
	{
		Repeat (' ', 2 * level);
		_out << "  <li>";
	}
	void EndListItem ()
	{
		_out << "</li>" << std::endl;
	}
	void BeginHeading (unsigned i)
	{
		_out << "<h" << i << ">";
	}
	void EndHeading (unsigned i)
	{
		_out << "</h" << i << ">" << std::endl;
	}
	void HorizontalRule ()
	{
		_out << "\n<hr/>" << std::endl;
	}
	void LessThan ()
	{
		_out << "&lt;";
	}
	void Amp ()
	{
		_out << "&amp;";
	}
	void Emph (unsigned count);

	virtual void InPlaceLink (std::string const & wikiUrl) = 0;
	virtual void InternalLink (std::string const & wikiUrl, std::string const & wikiText) = 0;
	virtual void ExternalLink (std::string const & wikiUrl, std::string const & wikiText) = 0;
	virtual void InternalDeleteLink (std::string const & wikiUrl) = 0;

	virtual void SqlCmd (Sql::Command const & cmd) = 0;
	virtual void SqlSelectCmd (Sql::SelectCommand const & cmd) = 0;
	virtual void SqlFromCmd (Sql::FromCommand const & cmd) = 0;
	virtual void ListImages (bool isSystem) = 0;

	virtual QueryString	const * GetQueryString () const { return 0; }
	virtual void QueryField (std::string const & fieldName) = 0;
	Sql::TuplesMap & GetTuplesMap () { return _tuples; }
	void TableField (std::string const & fieldName);
	void RegistryField (std::string const & fieldName);
protected:
	std::ostream &			_out;
private:
	std::vector<unsigned>	_emph;
	Sql::TuplesMap &		_tuples;
};

#endif
