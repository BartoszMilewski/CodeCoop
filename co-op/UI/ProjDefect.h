#if !defined PROJECT_DEFECT_H
#define PROJECT_DEFECT_H
//------------------------------------
//  (c) Reliable Software, 1997 - 2008
//------------------------------------

#include <Ctrl/Button.h>
#include <Win/Dialog.h>

class ProjDefectData
{
public:
    ProjDefectData (std::string const & projectName)
        : _kind (DeleteNothing),
		  _projectName (projectName),
		  _force (false)
    {}

    void SetDontDelete  	       () { _kind = DeleteNothing; }
	void SetDeleteProjectFiles     () { _kind = DeleteFiles; }
    void SetDeleteWholeProjectTree () { _kind = DeleteWholeTree; }
	void SetUnconditionalDefect ()    { _force = true; }

    bool DontDelete             () const { return _kind == DeleteNothing; }
	bool DeleteProjectFiles     () const { return _kind == DeleteFiles; }
    bool DeleteWholeProjectTree () const { return _kind == DeleteWholeTree; }
	bool IsUnconditionalDefect  () const { return _force; }

	std::string const & GetProjectName () const { return _projectName; }

private:
	enum DefectKind
	{
		DeleteNothing,  // Delete only project database
		DeleteFiles,    // Delete project database and project files
		DeleteWholeTree // Delete project database and whole project folder
	};

	DefectKind	_kind;
	std::string	_projectName;
	bool		_force;
};

class ProjDefectCtrl : public Dialog::ControlHandler
{
public:
    ProjDefectCtrl (ProjDefectData * data);

    bool OnInitDialog () throw (Win::Exception);
    bool OnApply () throw ();

	bool GetDataFrom (NamedValues const & source);

private:
    Win::RadioButton	_deleteHistory;
	Win::RadioButton	_deleteFiles;
    Win::RadioButton	_deleteAll;

    ProjDefectData *	_dlgData;
};

#endif
