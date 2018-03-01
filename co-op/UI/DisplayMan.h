#if !defined (DISPLAYMAN_H)
#define DISPLAYMAN_H
//----------------------------------
// (c) Reliable Software 1998 - 2006
//----------------------------------

#include "GlobalId.h"
#include "Table.h"

class WindowIter;
class TableBrowser;
class FileFilter;
class NocaseSet;
class UniqueName;
class Restriction;
class Vpath;

enum ViewPage
{
	BrowserPage, // added dynamically
    FilesPage,
    CheckInAreaPage,
    MailBoxPage,
    SynchAreaPage,
    HistoryPage,
	ProjectPage,
	ProjectMergePage,
	LastPage
};

// Display Manager is used by the Commander, both in the GUI and in the server mode
// In server mode it doesn't do anything

class DisplayManager
{
public:
    virtual void SelectPage (ViewPage page) {}
	virtual void SwitchPage (bool isForward) {}
	virtual void ClosePage (ViewPage page) {}
    virtual void Refresh (ViewPage page) {}
	virtual void RefreshPane (ViewPage page, Table::Id id) {}
	virtual void SetFileFilter (ViewPage page, std::unique_ptr<FileFilter> fileFilter) {}
	virtual bool IsEmptyPage (ViewPage page) const { return false; }
	// hierarchy operations
	virtual void GoUp () {}
	virtual void GoDown (std::string const & name) {}
	virtual void GotoRoot () {}
	virtual void GoTo(Vpath const & vpath) {}
	virtual void Navigate (std::string const & target, int scrollPos) {}

    virtual void RefreshAll () {}
	virtual void Rebuild (bool isWiki) {}
    virtual void ClearAll (bool forGood) {}
    virtual ViewPage GetCurrentPage () const { return FilesPage; }
	virtual Table::Id GetCurrentTableId () const { return Table::projectTableId; }
	virtual Table::Id GetMainTableId () const { return Table::projectTableId; }
	virtual Restriction const & GetPresentationRestriction () const { return *((Restriction const *)0); }
	virtual void GetScrollBookmarks (std::vector<Bookmark> & bookmakrs) const {}
	virtual void SetRestrictionFlag (std::string const & flagName, bool flagValue) {}
	virtual void SetExtensionFilter (NocaseSet const & extFilter) {}
	virtual void SetScrollBookmarks (std::vector<Bookmark> const & bookmakrs) {}
	virtual void SetInterestingItems (GidSet const & itemIds) {}

    virtual void InPlaceEdit (int row) {}
    virtual void BeginNewFolderEdit () {}
	virtual void AbortNewFolderEdit () {}
	virtual bool IsFilterEdited () const { return false; }
	
	// return true if allowed
	virtual bool CanShowHierarchy (bool & isVisible) const { return false; }
	virtual void ToggleHierarchyView () {}
	virtual bool CanShowDetails (bool & isVisible) const { return false; }
	virtual void ToggleDetailsView () {}
};

#endif
