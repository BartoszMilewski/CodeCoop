#if !defined (MERGEINSTRUMENTBAR_H)
#define MERGEINSTRUMENTBAR_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

#include "ButtonTable.h"
#include "DynamicRebar.h"
#include "Controllers.h"

#include <Win/Win.h>
#include <Ctrl/InfoDisp.h>

namespace Cmd { class Vector; }

class MergeInstrumentBar : public Tool::DynamicRebar
{
public:
	MergeInstrumentBar (Win::Dow::Handle parentWin,
						 Cmd::Vector & cmdVector,
						 Cmd::Executor & executor);

	Control::Handler * GetControlHandler (Win::Dow::Handle winFrom, 
										  unsigned idFrom) throw ();
	void SetFont (Font::Descriptor const & font);
	void RefreshTextField (std::string const & text);

	// Notify::RebarHandler
	bool OnChildSize (unsigned bandIdx,
					  unsigned bandId,
					  Win::Rect & newChildRect,
					  Win::Rect const & newBandRect) throw ();

private:
	void CreateToolBands (Cmd::Vector & cmdVector);
	void CreateTargetProjectTool (Win::Dow::Handle parentWin, Cmd::Executor & executor);
	void CreateMergeTypeTool (Win::Dow::Handle parentWin, Cmd::Executor & executor);
	void CreateDisplayTool (Win::Dow::Handle parentWin, Cmd::Executor & executor);

private:
	std::unique_ptr<TargetProjectCtrl>	_targetProjectCtrl;
	std::unique_ptr<MergeTypeCtrl>		_mergeTypeCtrl;
	std::unique_ptr<InfoDisplayCtrl>		_scriptCommentDisplay;
};

#endif
