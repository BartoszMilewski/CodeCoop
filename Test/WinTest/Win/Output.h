#if !defined (OUTPUT_H)
#define OUTPUT_H
//-----------------------------------------
// (c) Reliable Software 1998 -- 2002
//-----------------------------------------

#include <Win/Win.h>

namespace Win 
{
	class Exception;
	class CritSection;
}

namespace Out
{
	enum Buttons
	{
		Ok		         = MB_OK,
		OkCancel		 = MB_OKCANCEL,
		RetryCancel		 = MB_RETRYCANCEL,
		YesNo			 = MB_YESNO,
		YesNoCancel		 = MB_YESNOCANCEL,
		AbortRetryIgnore = MB_ABORTRETRYIGNORE
	};
	enum Answer
	{
		Yes		= IDYES,
		No		= IDNO,
		OK		= IDOK,
		Cancel  = IDCANCEL,
		Abort	= IDABORT,
		Retry	= IDRETRY,
		Ignore  = IDIGNORE,
		Close	= IDCLOSE,
		Help	= IDHELP
	};
	enum Severity
	{
		Information = MB_ICONINFORMATION,
		Question	= MB_ICONQUESTION,
		Warning		= MB_ICONWARNING,
		Error		= MB_ICONERROR
	};

	class PromptStyle
	{
	public:
		PromptStyle (Buttons buttons = YesNoCancel, 
					 Answer defaultAnswer = Yes,
					 Severity sev = Question)
			: _buttons (buttons),
		      _defaultAnswer (defaultAnswer),
			  _sev (sev)
		{}
		operator unsigned () const throw () 
		{ 
			return _buttons | _sev | CalcDefButton (); 
		}

	private:
		unsigned CalcDefButton () const throw ();

		Buttons  _buttons; 
		Answer   _defaultAnswer; 
		Severity _sev;
	};
				
	class Sink
	{
	public:
		Sink ()
			: _topWin (0), 
			  _appName ("Application"),
			  _foreground (false),
			  _verbose (true)
		{}
		void Init (Win::Dow::Handle topWin, char const * appName, bool foreground = true)
		{
			_topWin  = topWin;
			_appName = appName;
			_foreground = foreground;
		}
		void SetVerbose (bool flag) { _verbose = flag; }
		void Display (char const * msg, 
					  Severity sev = Information, 
					  Win::Dow::Handle owner = 0) const throw ();
		void DisplayModal (char const * msg, 
					  Severity sev = Information) const throw ();
		void Display (Win::Exception const & ex) const throw ();
		Answer Prompt (char const * question, 
					   PromptStyle const style = PromptStyle (), 
					   Win::Dow::Handle owner = 0) const throw ();
		Answer PromptModal (char const * question, 
					   PromptStyle const style = PromptStyle ()) const throw ();

		static std::string FormatExceptionMsg (Win::Exception const & ex); 
		static void DisplayException (Win::Exception const & ex, 
									  Win::Dow::Handle owner, 
									  char const * appName = "Application",
									  char const * title = "Internal problem", 
									  int flags = MB_OK | MB_ICONEXCLAMATION) throw ();

	private:
		Win::Dow::Handle _topWin;
		char const * 	_appName;
		bool			_foreground;
		bool			_verbose;
	};
};

#endif
