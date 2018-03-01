#if !defined (INSTRUMENTBAR_H)
#define INSTRUMENTBAR_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

#include "DynamicRebar.h"

#include <Win/Win.h>
#include <Ctrl/InfoDisp.h>

namespace Cmd { class Vector; }

class InstrumentBar : public Tool::DynamicRebar
{
public:
	InstrumentBar (Win::Dow::Handle parentWin,
				   Cmd::Vector & cmdVector,
				   Cmd::Executor & executor);

	Control::Handler * GetControlHandler (Win::Dow::Handle winFrom, 
										  unsigned idFrom) throw ();
	void SetFont (Font::Descriptor const & font);
	void RefreshFileDetails (std::string const & summary, std::string const & fullInfo);

	// Notify::RebarHandler
	bool OnChildSize (unsigned bandIdx,
					  unsigned bandId,
					  Win::Rect & newChildRect,
					  Win::Rect const & newBandRect) throw ();

private:
	void CreateToolBands (Cmd::Vector & cmdVector);
	void CreateDisplayTools (Win::Dow::Handle parentWin);

private:
	std::string			_shortFileInfo;
	Win::InfoDisplay	_fileDatailsDisplay;
};

#endif
