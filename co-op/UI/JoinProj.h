#if !defined JOINPROJ_H
#define JOINPROJ_H
//------------------------------------
//  (c) Reliable Software, 1997 - 2008
//------------------------------------

#include <Ctrl/Edit.h>
#include <Win/Dialog.h>

class JoinProjectData;

// The only purpose of this controller is to retrieve project join arguments
// from the command line string. Don't use it to display project join dialog!
class JoinProjectCtrl : public Dialog::ControlHandler
{
public:
    JoinProjectCtrl (JoinProjectData & joinData)
		: Dialog::ControlHandler (-1),
		  _joinData (joinData)
	{}

	bool GetDataFrom (NamedValues const & source);
	bool OnApply () throw () { return true; }

private:
    JoinProjectData &	_joinData;
};

// prompts user for path during command-line Join

class JoinPathData
{
public:
	JoinPathData (char const * defaultPath = 0)
		: _path (defaultPath)
	{}
	std::string const & GetPath () const { return _path; }
	void SetPath (std::string const & path) { _path = path; }

	bool IsValid () const;
    void DisplayErrors (Win::Dow::Handle winOwner) const;
private:
	std::string _path;
};

class JoinPathCtrl : public Dialog::ControlHandler
{
public:
    JoinPathCtrl (JoinPathData * data);

    bool OnInitDialog () throw (Win::Exception);
    bool OnApply () throw ();

private:
    Win::Edit		_path;
    JoinPathData *	_dlgData;
};

#endif
