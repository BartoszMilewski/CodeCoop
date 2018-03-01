// --------------------------
// (c) Reliable Software 2005
// --------------------------
#include <WinLibBase.h>
#include "Mime.h"
#include <Parse/NamedPair.h>

// Parse something like this
// Content-Type: application/octet-stream;
//	name="Scripts.znc"

void MIME::Headers::SetType (std::string const & type, std::string const & comment)
{
	unsigned curPos = 0;
	// type/subtype delimited by semicolon
	unsigned slashPos = type.find ('/', curPos);
	if (slashPos != std::string::npos)
	{
		_type = type.substr (curPos, slashPos - curPos);
		curPos = slashPos + 1;
		_subtype = type.substr (curPos);
	}
	else
	{
		_type = type.substr (curPos);
	}
	ParseComment (comment, _typeAttrib);
}

void MIME::Headers::SetDisposition (std::string const & disposition, std::string const & comment)
{
	_disposition = disposition;
	ParseComment (comment, _dispositionAttrib);
}

// Comment syntax:
// *(";" parameter) [unstructured_comment]
// parameter = name=value
// value may be quoted
void MIME::Headers::ParseComment (std::string const & str, MIME::Headers::Attributes & attrib)
{
	unsigned int const length = str.size ();
	unsigned int current = 0;
	while (current < length)
	{
		NamedPair<'='> attribute;
		unsigned endPair = str.find (';', current);
		if (endPair == std::string::npos)
		{
			attribute.Init (str.substr (current));
			current = length;
		}
		else
		{
			attribute.Init (str.substr (current, endPair));
			current = endPair + 1;
		}
		if (attribute.GetName ().empty ())
		{
			// unstructured_comment
			return;
		}
		else
		{
			attrib [attribute.GetName ()] = attribute.GetValue ();
		}
	}
}
