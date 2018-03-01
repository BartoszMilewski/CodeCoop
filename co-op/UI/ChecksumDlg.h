#if !defined (CHECKSUMDLG_H)
#define CHECKSUMDLG_H
//------------------------------------
//  (c) Reliable Software, 1999 - 2007
//------------------------------------

#include "VerificationReport.h"
#include "ProjectPath.h"

#include <Ctrl/Button.h>
#include <Ctrl/ListView.h>
#include <Win/Dialog.h>

class ChecksumMismatchData
{
public:
	ChecksumMismatchData (VerificationReport const & report, Project::Path & projectPath, bool isCheckout)
		: _corruptedFile (report.GetSequencer (VerificationReport::Corrupted)),
		  _projectPath (projectPath),
		  _isCheckout (isCheckout),
		  _repair (true),
		  _advanced (false)
    {}

	void SetRepair (bool flag) { _repair = flag; }
	void SetAdvanced (bool flag) { _advanced = flag; }

	VerificationReport::Sequencer GetSequencer () const { return _corruptedFile; }
	char const * GetRelativePath (GlobalId gid) { return _projectPath.MakePath (gid); }

	bool IsCheckout () const { return _isCheckout; }
	bool IsRepair () const { return _repair; }
	bool IsAdvanced () const { return _advanced; }

private:
	VerificationReport::Sequencer	_corruptedFile;
	Project::Path &					_projectPath;
    bool							_isCheckout;
	bool							_repair;
	bool							_advanced;
};

class ChecksumMismatchCtrl : public Dialog::ControlHandler
{
public:
    ChecksumMismatchCtrl (ChecksumMismatchData & data);

    bool OnInitDialog () throw (Win::Exception);
    bool OnApply () throw ();

private:
	Win::ReportListing		_fileList;
	Win::RadioButton		_repair;
	Win::RadioButton		_advanced;
    ChecksumMismatchData &	_dlgData;
};

#endif
