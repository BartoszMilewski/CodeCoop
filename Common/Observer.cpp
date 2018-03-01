//------------------------------------
//  (c) Reliable Software, 2005 - 2006
//------------------------------------
#include "precompiled.h"
#include "Observer.h"
#include <Sys/Thread.h>

void Notifier::Detach (Observer * observer)
{
	Container::iterator it = _topicObservers.begin (),
		end = _topicObservers.end ();
	while (it != end)
	{
		if (it->second == observer)
		{
			_topicObservers.erase (it);
			break;
		}
		++it;
	}
}

void Notifier::Notify ()
{
	// make a copy, in case the update destroys us (happens!)
	std::vector<Observer *> tmp;
	Gather (tmp, "");

	for (std::vector<Observer *>::iterator it = tmp.begin (); it != tmp.end (); ++it)
	{
		(*it)->UpdateAll ();
	}
}

void Notifier::Notify (std::string const & topic)
{
	// make a copy, in case the update destroys us (happens!)
	std::vector<Observer *> tmp;
	Gather (tmp, topic);

	for (std::vector<Observer *>::iterator it = tmp.begin (); it != tmp.end (); ++it)
	{
		(*it)->Update (topic);
	}
}

void Notifier::Notify (std::vector<RowChange> const & change)
{
	// make a copy, in case the update destroys us (happens!)
	std::vector<Observer *> tmp;
	Gather (tmp, "");

	for (std::vector<Observer *>::iterator it = tmp.begin (); it != tmp.end (); ++it)
	{
		(*it)->Update (change);
	}
}

void Notifier::NotifyIfNecessary (char const * topic)
{
	// make a copy, in case the update destroys us (happens!)
	std::vector<Observer *> tmp;
	Gather (tmp, "");

	for (std::vector<Observer *>::iterator it = tmp.begin (); it != tmp.end (); ++it)
	{
		(*it)->UpdateIfNecessary (topic);
	}
}

void Notifier::Gather (std::vector<Observer *> & result, std::string const & topic)
{
	Container::iterator it = _topicObservers.lower_bound (topic),
		end = _topicObservers.upper_bound (topic);
	while (it != end)
	{
		result.push_back (it->second);
		++it;
	}
}

// Transaction Notifier

TransactionNotifier::TransactionNotifier ()
	:_threadId (Thread::GetCurrentId ())
{}

void TransactionNotifier::Attach (TransactionObserver * observer) const
{
	if (Thread::GetCurrentId () != _threadId)
		return;
#if 0
	Container::iterator it = _observers.begin (),
		end = _observers.end ();
	while (it != end)
	{
		TransactionObserver const * pO = *it;
		dbg << "    " << pO << std::endl;
		//if (pO == observer)
		//	return;
		++it;
	}
	dbg << "Attaching observer: " << observer << std::endl;
#endif
	_observers.push_back (observer);
}

void TransactionNotifier::Detach (TransactionObserver * observer) const
{
	// dbg << "Detaching observer: " << observer << std::endl;
	Container::iterator it = _observers.begin (),
		end = _observers.end ();
	while (it != end)
	{
		if (*it == observer)
		{
			_observers.erase (it);
			break;
		}
		++it;
	}
}

bool TransactionNotifier::IsObserver (TransactionObserver * observer) const
{
	return std::find (_observers.begin (), _observers.end (), observer) != _observers.end ();
}

void TransactionNotifier::StartNotifying () const
{
	Container::iterator it = _observers.begin (), end = _observers.end ();
	while (it != end)
	{
		(*it)->StartUpdate ();
		++it;
	}
}

void TransactionNotifier::AbortNotifying () const
{
	Container::iterator it = _observers.begin (), end = _observers.end ();
	while (it != end)
	{
		(*it)->AbortUpdate ();
		++it;
	}
}

// This notification is not delayed, it might unregister observers

void TransactionNotifier::CommitNotifications (bool delayRefresh) const
{
	TmpContainer observers;
	Gather (observers);
	TmpContainer::iterator it = observers.begin (), end = observers.end ();
	while (it != end)
	{
		if (IsObserver (*it))
		{
			// dbg << "Commit observer " << *it << std::endl;
			(*it)->CommitUpdate (delayRefresh);
		}
		++it;
	}
}

// Note: these notifications are delayed--they don't invalidate observers

void TransactionNotifier::Notify (ChangeKind kind, GlobalId gid)
{
	Container::iterator it = _observers.begin (), end = _observers.end ();
	while (it != end)
	{
		(*it)->Update (kind, gid);
		++it;
	}
}

void TransactionNotifier::Notify (ChangeKind kind, char const * name)
{
	Container::iterator it = _observers.begin (), end = _observers.end ();
	while (it != end)
	{
		(*it)->Update (kind, name);
		++it;
	}
}

void TransactionNotifier::Notify (ChangeKind kind, GlobalId gid, char const * name)
{
	Container::iterator it = _observers.begin (), end = _observers.end ();
	while (it != end)
	{
		(*it)->Update (kind, gid, name);
		++it;
	}
}

// This notification is not delayed, it might unregister observers

void TransactionNotifier::ExternalNotify (char const * topic)
{
	// ExternalUpdate might unregister an observer
	// We have to make a temporary copy of observers
	TmpContainer observers;
	Gather (observers);
	typedef std::vector<TransactionObserver *> ObsCont;
	for (TmpContainer::iterator iter = observers.begin (); iter != observers.end (); ++iter)
	{
		if (IsObserver (*iter))
			(*iter)->ExternalUpdate (topic);
	}
}

void TransactionNotifier::Gather (TmpContainer & observers) const
{
	for (Container::iterator iter = _observers.begin (); iter != _observers.end (); ++iter)
		observers.push_back (*iter);
}
