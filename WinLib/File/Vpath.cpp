//-----------------------------------
// (c) Reliable Software, 2003 - 2007
//-----------------------------------

#include <WinLibBase.h>
#include "Vpath.h"


Vpath::Vpath (Vpath const & vpath)
{
	std::copy (vpath.begin (),
		vpath.end (),
		std::back_insert_iterator<std::vector<std::string> > (_segments));
}

Vpath & Vpath::operator = (Vpath const & vpath)
{
	_segments.clear ();
	std::copy (vpath.begin (),
		vpath.end (),
		std::back_insert_iterator<std::vector<std::string> > (_segments));
	return *this;
}

void Vpath::SetPath (std::string const & strPath)
{
	_segments.clear ();
	unsigned idx = 0;
	unsigned len = strPath.size ();
	while (idx < len)
	{
		unsigned idxEnd = FindSeparator(strPath, idx);
		unsigned count = (idxEnd != std::string::npos)? idxEnd - idx: std::string::npos;
		_segments.push_back (strPath.substr (idx, count));
		if (count == std::string::npos)
			break;
		idx = SkipSeparator(strPath, idxEnd);
	}
}

std::string Vpath::ToString () const
{
	Vpath::const_iterator it = begin ();
	if (it == end ())
		return std::string ();

	std::string strPath (*it);
	++it;
	while (it != end ())
	{
		strPath += GetSeparator();
		strPath += *it;
		++it;
	}
	return strPath;
}

std::string Vpath::GetFilePath (std::string const & fileName)
{
	AppendFileName (fileName);
	std::string result = ToString ();
	RemoveFileName ();
	return result;
}