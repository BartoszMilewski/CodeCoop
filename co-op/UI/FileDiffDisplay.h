#if !defined (FILEDIFFDISPLAY_H)
#define FILEDIFFDISPLAY_H
//---------------------------------------
//  (c) Reliable Software, 2002 -- 2004
//---------------------------------------

#include "GlobalId.h"

#include <Ctrl/ListView.h>
#include <Graph/ImageList.h>

class FileDisplayTable;
class NotificationSink;

class FileDiffDisplay : public Win::ReportListing, public Notify::ListViewHandler
{
public:
	FileDiffDisplay (unsigned ctrlId, 
					 FileDisplayTable & fileTable, 
					 NotificationSink * notificationSink = 0);
	~FileDiffDisplay ();
	void Init (Win::Dow::Handle winParent, int id);
	void HideUnchanged (bool flag) { _hideUnchanged = flag; }

	bool OnDblClick () throw ();
	bool OnItemChanged (Win::ListView::ItemState & state) throw ();
	bool OnKeyDown (int key) throw ();
	void Refresh ();
	void OpenSelected ();
	void OpenAll ();
	void CopyToClipboard (Win::Dow::Handle dlgWin);

	class SelectionIterator
	{
	public:
		SelectionIterator (Win::ReportListing const & report)
			: _report (report)
		{
			_cur = _report.GetFirstSelected ();
		}

		void Advance () { _cur = _report.GetNextSelected (_cur); }
		bool AtEnd () const { return _cur == -1; }

		GlobalId GetGlobalId () const { return _report.GetItemParam (_cur); }

	private:
		Win::ReportListing const &	_report;
		int							_cur;
	};

private:
    enum
    {
        colName,
        colPath,
		colGid,
		colType,
		colState
    };

private:
	ImageList::AutoHandle	_images;
	FileDisplayTable &		_fileTable;
	NotificationSink *		_notificationSink;
	bool					_hideUnchanged;
};

// To get notifications from FileDiffDisplay derive from NotificationSink
// and overwriting some of its methods
class NotificationSink
{
public:
	virtual ~NotificationSink () {}

	virtual void OnDblClick () throw () {}
	virtual void OnGetDispInfo (Win::ListView::Request const & request,
								Win::ListView::State const & state,
								Win::ListView::Item & item) throw () {}
	virtual void OnItemChanged (Win::ListView::ItemState & state) throw () {}
	virtual void OnBeginLabelEdit (long & result) throw () {}
	virtual void OnEndLabelEdit (Win::ListView::Item * item, long & result) throw () {}
	virtual void OnColumnClick (int col) throw () {}
	virtual void OnKeyDown (int key) throw () {}
};

#endif
