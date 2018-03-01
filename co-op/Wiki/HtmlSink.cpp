//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------
#include "precompiled.h"
#include "HtmlSink.h"
#include "Registry.h"

HtmlSink::~HtmlSink ()
{}

void HtmlSink::Copy (std::string::const_iterator begin, std::string::const_iterator end)
{
	std::copy (begin, end, std::ostream_iterator<char> (_out));
}

void HtmlSink::Emph (unsigned count)
{
	Assert (count > 1 && count <= 4);
	if (_emph.empty () || count != _emph.back ())
	{
		switch (count)
		{
		case 2:
			_out << "<i>";
			break;
		case 3:
			_out << "<b>";
			break;
		case 4:
			_out << "<i><b>";
			break;
		}
		_emph.push_back (count);
	}
	else
	{
		Assert (!_emph.empty () && count == _emph.back ());
		switch (_emph.back ())
		{
		case 2:
			_out << "</i>";
			break;
		case 3:
			_out << "</b>";
			break;
		case 4:
			_out << "</b></i>";
			break;
		}
		_emph.pop_back ();
	}
}

void HtmlSink::TableField (std::string const & fieldName)
{
	if (_tuples.find (fieldName) == _tuples.end ())
		_out << "\"\"";
	else
		_out << _tuples [fieldName];
}

void HtmlSink::RegistryField (std::string const & fieldName)
{
	Registry::UserWikiPrefs prefs;
	std::string value = prefs.GetPropertyValue (fieldName);
	if (value.size () != 0)
	{
		_out << value;
	}
	else
	{
		_out << "<span style='color:red'> Warning: value of " << fieldName << " not found in local registry</span> ";
	}
}