#if !defined (VERIFYREPORTDLG_H)
#define VERIFYREPORTDLG_H
//------------------------------------
//  (c) Reliable Software, 1999 - 2007
//------------------------------------

#include "VerificationReport.h"

#include <Win/Dialog.h>
#include <Ctrl/ListView.h>

namespace Project
{
	class Path;
};

class VerifyReportCtrl : public Dialog::ControlHandler
{
public:
    VerifyReportCtrl (VerificationReport const & report,
					  std::string const & projectName,
					  Project::Path	& relativePath);

    bool OnInitDialog () throw (Win::Exception);

private:
	void Display (VerificationReport::Sequencer seq,
				  std::string const & state,
				  std::string const & action);

private:
	Win::ReportListing			_fileListing;
    VerificationReport const &	_report;
	std::string const &			_projectName;
	Project::Path &				_projectPath;
};


#endif
