#if !defined (CMDLINEARGS_H)
#define CMDLINEARGS_H
//
// (c) Reliable Software 1998
//

#include "LightString.h"
#include <vector>

class CmdLineArgs
{
public:
	CmdLineArgs (char const * cmdLine)
	{
		int cmdLineLen = strlen (cmdLine);
		int delim = SkipToDelim (0, cmdLineLen, cmdLine);
		if (delim != -1)
		{
			if (cmdLine [delim] != ' ')
			{
				// Get switch
				delim++;
				int startDelim = delim;
				delim = SkipToDelim (startDelim, cmdLineLen, cmdLine);
				if (delim != -1)
				{
					std::string sw (&cmdLine [startDelim], delim - startDelim);
					_switch = sw;

					delim++;
					startDelim = delim;
					delim = SkipToDelim (startDelim, cmdLineLen, cmdLine);
					if (delim != -1)
					{
						// Get file name
						std::string fname (&cmdLine [startDelim], delim - startDelim);
						_fileName = fname;
					}
				}
			}
			else
			{
				// Get file name
				std::string fname (cmdLine, delim);
				_fileName = fname;
			}
		}
		// Get recipients list
		for (delim++; 0 < delim && delim < cmdLineLen; delim++)
		{
			int startDelim = delim;
			delim = SkipToDelim (startDelim, cmdLineLen, cmdLine);
			int recipientStrLen = delim != -1 ? delim - startDelim : cmdLineLen - startDelim;
			_recipients.push_back (std::string (&cmdLine [startDelim], recipientStrLen));
		}
	}

	bool AreOk () const { return _fileName.length () != 0 && _recipients.size () != 0; }
	bool IsVerbose () const  { return _switch == "Verbose"; }
	std::vector<std::string> const & GetRecipients () const { return _recipients; }
	std::string const & GetFileName () const { return _fileName; }

private:
	int	SkipToDelim (int start, int len, char const * line)
	{
		for (int i = start; i < len; i++)
		{
			if (line [i] == ' ' || line [i] == '/' || line [i] == '-')
				return i;
		}
		return -1;
	}
private:
	std::string	_fileName;
	std::string	_switch;
	std::vector<std::string> _recipients;
};

#endif
