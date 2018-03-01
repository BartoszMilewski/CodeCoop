#if !defined (MOVEPROJ_H)
#define MOVEPROJ_H
// ----------------------------------
// (c) Reliable Software, 2000 - 2008
// ----------------------------------

#include "ProjectBlueprint.h"

#include <File/Path.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Button.h>
#include <Win/Dialog.h>

class MoveProjectData : public Project::Blueprint
{
public:
	MoveProjectData (Catalog & catalog)
		: Project::Blueprint (catalog),
		  _onlyFiles (true),
		  _isVirtual (false)
	{}

	bool MoveProjectFilesOnly () const { return _onlyFiles; }
	bool IsVirtualMove () const { return _isVirtual; }

	void SetProjectFilesOnly (bool flag) { _onlyFiles = flag; }
	void SetVirtualMove (bool flag) { _isVirtual = flag; }
private:
	bool		_onlyFiles;
	bool		_isVirtual;
};

class MoveProjectCtrl : public Dialog::ControlHandler
{
public:
    MoveProjectCtrl (MoveProjectData & moveData);

    bool OnInitDialog () throw (Win::Exception);
    bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();
	
	bool GetDataFrom (NamedValues const & source);

private:
    Win::Edit			_newSrc;
    Win::Button			_browse;
	Win::RadioButton	_moveFiles;
    Win::RadioButton	_moveAll;
	MoveProjectData &	_moveData;
};

#endif
