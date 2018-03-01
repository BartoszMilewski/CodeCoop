#if !defined (FEEDBACKMAN_H)
#define FEEDBACKMAN_H
// ---------------------------
// (c) Reliable Software, 2006
// ---------------------------

#include <Com/TaskBarIcon.h>
#include <Graph/Icon.h>
#include <Win/Message.h>
#include <Sys/Synchro.h>

enum DispatchingActivity
{
	Idle,
	RetrievingEmail,
	Forwarding,
	SendingEmail
};

class FeedbackMan
{
public:
	FeedbackMan ();

	void Init (Win::Dow::Handle appWin);
	void RefreshScriptCount (int inCount, int outCount)
	{
		_inCount  = inCount;
		_outCount = outCount;
		ShowState ();
	}

	void ReShowTaskbarIcon ()
	{
		_taskbarIcon->ReShow ();
		ShowState ();
	}

	void DisplayAlert (std::string const & alert);

	int PushDispatchingActivity (DispatchingActivity activity)
	{
		int h = _activity.Push(activity);
		_appWin.PostMsg (_msgActivityChange);
		return h;
	}
	void PopDispatchingActivity(int h)
	{
		_activity.Pop(h);
		_appWin.PostMsg (_msgActivityChange);
	}
	void ShowActivity ();
	bool IsDispatchingActivity () {	return _activity.Get () != Idle; }
	void Animate ();

	void SetUpdateReady (char const * info);
	void ResetUpdateReady ();

private:
	void ShowState ();
private:
	class CritActivity
	{
	public:
		CritActivity() : _curHandle(0)  {}
		DispatchingActivity Get()
		{
			Win::Lock lock (_critSection);
			if (_activity.empty())
				return Idle;
			return _activity.back().second;
		}
		int Push(DispatchingActivity activity)
		{
			Win::Lock lock(_critSection);
			_activity.push_back(std::make_pair(_curHandle, activity));
			return _curHandle++;
		}
		void Pop(int h)
		{
			Win::Lock lock (_critSection);
			// Revisit: should use reverse iterator
			std::vector<std::pair<int, DispatchingActivity> >::iterator it;
			for (it = _activity.begin(); it != _activity.end(); ++it)
			{
				if (it->first == h)
				{
					_activity.erase(it);
					break;
				}
			}
		}
	private:
		int _curHandle;
		std::vector<std::pair<int, DispatchingActivity> >	_activity;
		Win::CritSection	    			_critSection;
	};
private:
	Icon::Handle	_alertNoScriptsIcon;
	Icon::Handle	_alertInScriptsIcon;
	Icon::Handle	_alertOutScriptsIcon;
	Icon::Handle	_alertInOutScriptsIcon;
	Icon::Handle	_alertActivityIcon;
	Icon::Handle	_errorIcon;
	Icon::Handle	_noScriptsIcon;
	Icon::Handle	_inScriptsIcon;
	Icon::Handle	_outScriptsIcon;
	Icon::Handle	_inOutScriptsIcon;

	Win::Dow::Handle			 _appWin;
	Win::RegisteredMessage const _msgActivityChange;

	std::unique_ptr<TaskbarIcon>	_taskbarIcon;

	CritActivity			_activity;
	std::string				_updateInfo;
	int						_inCount;
	int						_outCount;
};

class ActivityIndicator
{
public:
	ActivityIndicator (FeedbackMan & feedbackMan, DispatchingActivity activity)
		: _feedbackMan (feedbackMan)
	{
		Assert (activity != Idle);
		_handle = _feedbackMan.PushDispatchingActivity(activity);
	}
	~ActivityIndicator ()
	{
		_feedbackMan.PopDispatchingActivity(_handle);
	}
private:
	int _handle;
	FeedbackMan			 & _feedbackMan;
};

// Global Feedback Manager
extern FeedbackMan TheFeedbackMan;

#endif
