#if !defined (PROJECTCHECKER_H)
#define PROJECTCHECKER_H
//------------------------------------
//  (c) Reliable Software, 1999 - 2007
//------------------------------------

#include "VerificationReport.h"

namespace Progress
{
	class Dialog;
}
class Model;
namespace Progress { class Meter; }

class ProjectChecker
{
public:
	ProjectChecker (Model & model)
		: _model (model),
		  _missingFolders (false),
		  _extraRepairNeeded (false),
		  _hasBeenVerified(false)
	{}

	void Verify (Progress::Meter & overallMeter, Progress::Meter & specificMeter);
	bool MissingFolderFound () const { return _missingFolders; }
	bool IsFileRepairNeeded () const;
	bool Repair ();

private:
	Model &				_model;
	VerificationReport	_report;
	bool				_hasBeenVerified;
	bool				_missingFolders;
	bool				_extraRepairNeeded;
};

#endif
