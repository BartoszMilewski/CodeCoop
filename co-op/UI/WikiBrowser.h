#if !defined (WIKIBROWSER_H)
#define WIKIBROWSER_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

#include "WidgetBrowser.h"
#include "Table.h"

class RecordSet;
class WebBrowserView;

class WikiBrowser: public WidgetBrowser
{
public:
	WikiBrowser (TableProvider & tableProv, Table::Id tableId, WebBrowserView & view)
		: WidgetBrowser (tableProv, tableId), _view (view)
	{}
	~WikiBrowser ();

	std::string const & GetCurrentDir () { return _recordSet->GetRootName (); }
	std::string const & GetTmpDir () const { return _tmpDir; }
	std::string const & GetGlobalDir () const { return _globalDir; }

	bool Show (FeedbackManager & feedback);
	void Hide () {}
	void OnFocus ();
	void Invalidate ();
	void Clear (bool forGood);
	// Observer
	void UpdateAll () {}
	void UpdateIfNecessary (char const * topic);
private:
	std::unique_ptr<RecordSet> _recordSet;
	std::string _tmpDir;
	std::string _globalDir;
	WebBrowserView & _view;
};

#endif
