#if !defined (BUTTONTABLE_H)
#define BUTTONTABLE_H
//----------------------------------
// (c) Reliable Software 1997 - 2006
//----------------------------------

#include <Ctrl/ToolBar.h>
#include <Ctrl/Rebar.h>

namespace Tool
{
	static unsigned int const DifferButtons = 0;

	extern Tool::Item Buttons [];
	extern int const * LayoutTable [];

	static unsigned int const DifferBandId = 0;
	static unsigned int const FileDetailsBandId = 1;

	extern Tool::BandItem Bands [];
	extern unsigned int const * BandLayoutTable [];
};

#endif
