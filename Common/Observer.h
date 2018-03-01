#if !defined (OBSERVER_H)
#define OBSERVER_H
//------------------------------------
//  (c) Reliable Software, 1999 - 2006
//------------------------------------
#include "TableChange.h"

// Observer is notified by Notifier
// You register an observer with a Notifier in order to get notifications
// You can chain observers and notifiers, as in:
//    RecordSet -> TableBrowser -> View
// RecordSet is the Notifier (it is also a TransactionObserver)
// TableBrowser is the Observer of the RecordSet as well as the Notifier of the UiManager
// UiManager is the Observer of the TableBrowser

class Observer
{
public:
	virtual ~Observer () {}
	virtual void UpdateAll () = 0;
	virtual void Update (std::string const & topic) {}
	virtual void Update (std::vector<RowChange> const & change) {}
	virtual void UpdateIfNecessary (char const * topic) {}
protected:
	Observer () {}
};

class Notifier
{
	typedef std::multimap<std::string, Observer *> Container;
public:
	void Attach (Observer * observer, std::string const & topic)
	{
		_topicObservers.insert (std::make_pair (topic, observer));
	}
	void Attach (Observer * observer)
	{
		_topicObservers.insert (std::make_pair ("", observer));
	}
	void Detach (Observer * observer);
	void Notify ();
	void Notify (std::string const & topic);
	void Notify (std::vector<RowChange> const & change);
	void NotifyIfNecessary (char const * topic);
private:
	void Gather (std::vector<Observer *> & result, std::string const & topic);
private:
	Container	_topicObservers;
};

//
// TransactionObserver accumulates notifications from the Table
// and does something with them during CommitUpdate 
// (called by Table's CommitNotifications at the end of Commit)
//

class TransactionObserver
{
public:
	typedef std::vector<RowChange>::const_iterator ChangeIter;
public:
	TransactionObserver ()
		: _isValid (true)
	{}

	virtual ~TransactionObserver () {}

	void Invalidate ()
	{
		if (_isValid)
		{
			_isValid = false;
			_change.clear ();
		}
	}

	virtual RowChange TranslateNotification (TableChange const & change) = 0;
	void Update (ChangeKind kind, GlobalId gid)
	{
		if (_isValid)
		{
			TableChange change (kind, gid);
			RowChange row = TranslateNotification (change);
			_change.push_back (row);
		}
	}
	void Update (ChangeKind kind, char const * name)
	{
		if (_isValid)
		{
			TableChange change (kind, name);
			RowChange row = TranslateNotification (change);
			_change.push_back (row);
		}
	}
	void Update (ChangeKind kind, GlobalId gid, char const * name)
	{
		if (_isValid)
		{
			TableChange change (kind, gid, name);
			RowChange row = TranslateNotification (change);
			_change.push_back (row);
		}
	}
    virtual void StartUpdate ()
	{
		_change.clear ();
	}

	virtual void AbortUpdate () {}
	virtual void CommitUpdate (bool delayRefresh) = 0;
	virtual void ExternalUpdate (char const * topic) = 0;

protected:
	bool _isValid;
	std::vector<RowChange>	_change;
};

// Rejects observers from other threads!
class TransactionNotifier
{
	typedef std::list<TransactionObserver *> Container;
	// typedef std::vector<TransactionObserver *> Container;
	typedef std::vector<TransactionObserver *> TmpContainer;

public:
	TransactionNotifier ();
	void Attach (TransactionObserver * observer) const;
	void Detach (TransactionObserver * observer) const;

	bool IsObserver (TransactionObserver * observer) const;
	void StartNotifying () const;
	void AbortNotifying () const;
	void CommitNotifications (bool delayRefresh) const;

	void Notify (ChangeKind kind, GlobalId gid);
	void Notify (ChangeKind kind, char const * name);
	void Notify (ChangeKind kind, GlobalId gid, char const * name);
	void ExternalNotify (char const * topic = 0);

private:
	void Gather (TmpContainer & observers) const;

private:
	unsigned long		_threadId;
	mutable Container	_observers;
};

#endif
