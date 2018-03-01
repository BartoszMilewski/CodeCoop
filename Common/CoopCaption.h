#if !defined (COOPCAPTION_H)
#define COOPCAPTION_H
//-----------------------------------------
//  (c) Reliable Software, 2000-2003
//-----------------------------------------

#include <sstream>

class CoopCaption
{
public:
	CoopCaption (std::string const & projName, std::string const & projRoot)
	{
		_caption << projName << " (" << projRoot << ")";
	}
	std::string str () { return _caption.str (); }
private:
	std::ostringstream _caption;
};

#endif

