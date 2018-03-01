#if !defined (INSTRUMENTBAR_H)
#define INSTRUMENTBAR_H
//------------------------------------
//  (c) Reliable Software, 2005 - 2007
//------------------------------------

#include "ButtonTable.h"
#include "DynamicRebar.h"
#include "Controllers.h"
#include "WikiController.h" // UrlController

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
	void RefreshTextField (std::string const & text);
	void LayoutChange (Tool::BarLayout newLayout);
	void SetFont (Font::Descriptor const & font);

	// Notify::RebarHandler
	bool OnChildSize (unsigned bandIdx,
					  unsigned bandId,
					  Win::Rect & newChildRect,
					  Win::Rect const & newBandRect) throw ();
	std::string GetEditText () const;
private:
	void CreateToolBands (Cmd::Vector & cmdVector);
	void CreateHistoryFilterTool (Win::Dow::Handle parentWin, Cmd::Executor & executor);
	void CreateUrlTool (Win::Dow::Handle parentWin, Cmd::Executor & executor);
	void CreateDisplayTools (Win::Dow::Handle parentWin);
	void RefreshScriptComment (std::string const & comment);
	void RefreshCurrentFolder (std::string const & path);
	void RefreshCheckinStatus (std::string const & status);
	void RefreshFilter (std::string const & caption);

private:
	Tool::BarLayout					_curBarLayout;
	std::unique_ptr<HistoryFilterCtrl> _dropDownCtrl;
	Win::InfoDisplay				_currentFolderDisplay;
	Win::InfoDisplay				_scriptCommentDisplay;
	Win::InfoDisplay				_checkinStatusDisplay;
	std::unique_ptr<UrlCtrl>			_urlDisplayCtrl;
};

#endif
