#if !defined (OUTPUT_H)
#define OUTPUT_H
//----------------------------------
// (c) Reliable Software 1998 - 2009
//----------------------------------

#include <File/Path.h>

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
		Sink ();
		void Init (char const * appName, FilePath const & logsFolder = FilePath ());
		void SetParent (Win::Dow::Handle topWin) { _topWin = topWin; }
		void ForceForeground () { _foreground = true; }

		void AddUncriticalSect (Win::CritSection & critSect)
		{
			_critSect = &critSect;
		}
		void Reset ()
		{
			_critSect = 0;
			_topWin = Win::Dow::Handle ();
		}
		bool SetVerbose (bool flag)
		{
			bool tmp = _verbose;
			_verbose = flag;
			return tmp;
		}
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

		void LogFile (std::string const & srcFilePath,
					  std::string const & destFilename, 
					  std::string const & subfolderRelPath = std::string ()) throw ();

		void LogNote (std::string const & logFilename, std::string const & note) throw ();

	private:
		Win::Dow::Handle _topWin;
		std::string		_appName;
		bool			_foreground;
		bool			_verbose;
		Win::CritSection * _critSect; // un-critical section (released during display)
		FilePath		_logsRoot;
	};

	class Muter
	{
	public:
		Muter (Out::Sink & output, bool quiet)
			: _output (output),
			  _verbose (_output.SetVerbose (!quiet))
		{}
		~Muter ()
		{
			_output.SetVerbose (_verbose);
		}

	private:
		Out::Sink &	_output;
		bool		_verbose;
	};

};

#endif
