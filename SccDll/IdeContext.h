#if !defined (IDECONTEXT_H)
#define IDECONTEXT_H
//-----------------------------------------
//  (c) Reliable Software, 2000 -- 2006
//-----------------------------------------

#include <string>

typedef long (*IdeTextOutProc) (char const * msg, unsigned long);

class IDEContext
{
public:
	IDEContext (char const * callerName);

	bool IsBorlandIDE () const { return _isBorlandIDE; }
	bool IsIdeTextOutAvailable () const { return _ideTextOut != 0; }

	void SetIdeTextOut (IdeTextOutProc ideTextOut) { _ideTextOut = ideTextOut; }
	void SetProjectId (int projId) { _projectId = projId; }
	void SetProjectName (std::string const & name) { _projectName = name; }
	void SetRootPath (std::string const & path) { _rootPath = path; }
	void ResetCurrentProject ()
	{
		_projectId = -1;
		_projectName.clear ();
		_rootPath.clear ();
	}
	// Co-op project hint management
	bool IsProjectLocated () const { return _projectId != -1; }
	int GetProjectId () const { return _projectId; }
	char const * GetProjectName () const { return _projectName.c_str (); }
	char const * GetRootPath () const { return _rootPath.c_str (); }

	void Display (std::string const & msg, bool isError = false) const;

private:
	unsigned int	_version;
	// Code Co-op project identification hint
	// Valid only when one IDE project corresponds to one Code Co-op project
	bool			_isBorlandIDE;
	int				_projectId;
	std::string		_projectName;
	std::string		_rootPath;
	IdeTextOutProc	_ideTextOut;
};

#endif
