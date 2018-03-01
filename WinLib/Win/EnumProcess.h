#if !defined (ENUMPROCESS_H)
#define ENUMPROCESS_H
//----------------------------------------------------
// (c) Reliable Software 2000 -- 2003
//----------------------------------------------------

namespace Win
{
	class Messenger
	{
	public:
		virtual void DeliverMessage (Win::Dow::Handle win) = 0;
	};

	class EnumProcess
	{
	public:
		EnumProcess (Win::Dow::Handle callerWindow, Messenger & messenger);
		EnumProcess (std::string const & className, Messenger & messenger);

	private:
		void ProcessWindow (Win::Dow::Handle win);
		static BOOL CALLBACK EnumCallback (HWND hwnd, LPARAM lParam);

	private:
		Win::Dow::Handle	_callerWindow;
		std::string			_className;
		Messenger &			_messenger;
	};

	template<class P>
	class ProcessFinder
	{
	public:
		ProcessFinder ()
			: _predicate (0)
		{}
		bool Find (P & predicate);
		Win::Dow::Handle GetWin () const
		{
			return _win;
		}
	private:
		static BOOL CALLBACK EnumCallback (HWND hwnd, LPARAM lParam);
		bool ProcessWindow (Win::Dow::Handle h);
	private:
		P * _predicate;
		Win::Dow::Handle _win;
	};
}

template<class P>
bool Win::ProcessFinder<P>::ProcessWindow (Win::Dow::Handle h)
{
	bool isFound = (*_predicate) (h);
	if (isFound)
		_win = h;
	return isFound;
}

template<class P>
bool Win::ProcessFinder<P>::Find (P & predicate)
{
	_predicate = &predicate;
	_win.Reset ();
	::EnumWindows (&EnumCallback, reinterpret_cast<LPARAM>(this));
	return !_win.IsNull ();
}

template<class P>
BOOL CALLBACK Win::ProcessFinder<P>::EnumCallback (HWND hwnd, LPARAM lParam)
{
	Assert (lParam != 0);
	ProcessFinder * finder = reinterpret_cast<ProcessFinder *>(lParam);
	return !finder->ProcessWindow (hwnd);
}

#endif
