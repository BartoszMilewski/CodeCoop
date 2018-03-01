// (c) Reliable Software 2002
#include <WinLibBase.h>
#include <Win/WinMain.h>
#include <StringOp.h>

// Windows entry point: calls clent entry point

int WINAPI WinMain
	(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdParam, int cmdShow)
{
	return Win::Main (hInst, cmdParam, cmdShow);
}

Win::CommandLine::CommandLine ()
{
	std::string cmdLine (::GetCommandLine ());
	if (cmdLine.size () == 0)
		return;
	unsigned i = 0;
	if (cmdLine [0] == '"')
	{
		i = cmdLine.find ('"', 1);
		if (i == std::string::npos)
			return; // wrong: missing closing "
		// without the quotes
		_exePath = cmdLine.substr (1, i - 1);
	}
	else
	{
		i = cmdLine.find (' ');
		_exePath = cmdLine.substr (0, i);
	}

	if (i + 1 < cmdLine.size ())
	{
		i = cmdLine.find_first_not_of (' ', i + 1);
		if (i != std::string::npos)
		{
			_args = cmdLine.substr (i);
		}
	}
}


