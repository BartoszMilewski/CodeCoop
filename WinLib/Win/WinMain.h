#if !defined (WINMAIN_H)
#define WINMAIN_H
// (c) Reliable Software 2002
#include <Win/Instance.h>

namespace Win
{
	// Client must implement this entry point in every Windows program
	int Main (Win::Instance inst, char const * cmdParam, int cmdShow);

	class CommandLine
	{
	public:
		CommandLine ();
		// string ending with .exe
		std::string const & GetExePath () const { return _exePath; }
		std::string const & GetArgs () const { return _args; }
	private:
		std::string _exePath;
		std::string _args;
	};
}

#endif
