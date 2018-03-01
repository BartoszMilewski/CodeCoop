#if !defined (BUTTONTABLE_H)
#define BUTTONTABLE_H
//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include <Ctrl/ToolBar.h>
#include <Ctrl/Rebar.h>

namespace Tool
{
	enum BarLayout
	{
		// Main instrument bar layouts
		NotInProject,
		BrowseProjects,
		BrowseWiki,
		BrowseFiles,
		BrowseMailbox,
		BrowseCheckInArea,
		BrowseSyncArea,
		BrowseHistory,
		BrowseProjectMerge,

		// Auxilary instrument bar layouts
		HistoryScriptDetails,	// In the mailbox and history views
		ProjectMergeTools		// In the project merge view
	};

	extern Tool::Item Buttons [];
	extern int const * ButtonLayoutTable [];

	// Instrument band ids - bands can appear on any instrument bar
	static unsigned int const NotInProjectBandId = 0;
	static unsigned int const FilesBandId = 1;
	static unsigned int const CheckinBandId = 2;
	static unsigned int const MailboxBandId = 3;
	static unsigned int const SyncBandId = 4;
	static unsigned int const HistoryBandId = 5;
	static unsigned int const ProjectBandId = 6;
	static unsigned int const ScriptCommentBandId = 7;
	static unsigned int const HistoryFilterBandId = 8;
	static unsigned int const CurrentFolderBandId = 9;
	static unsigned int const ScriptDetailsBandId = 10;
	static unsigned int const BrowserBandId = 11;
	static unsigned int const UrlBandId = 12;
	static unsigned int const TargetProjectBandId = 13;
	static unsigned int const MergedVersionBandId = 14;
	static unsigned int const ProjectMergeBandId = 15;
	static unsigned int const MergeTypeBandId = 16;
	static unsigned int const MergeDetailsBandId = 17;
	static unsigned int const CheckinStatusBandId = 18;

	extern Tool::BandItem Bands [];
	extern Tool::BandItem ScriptBands [];
	extern Tool::BandItem ProjectMergeBands [];
	extern unsigned int const * BandLayoutTable [];
};

#endif
