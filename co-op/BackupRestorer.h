#if !defined (BACKUPRESTORER_H)
#define BACKUPRESTORER_H
//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "ProjectPathVerifier.h"

class Model;
class BackupMarker;
namespace Progress
{
	class Meter;
}
namespace Project
{
	class Data;
}

class BackupRestorer
{
public:
	BackupRestorer (Model & model);

	void RestoreRootPaths (Progress::Meter & meter);
	void RequestVerification (Progress::Meter & meter);
	void RepairProjects (Progress::Meter & meter);
	void Summarize ();

private:
	void SetCurrentActivity (int projectId, Progress::Meter & meter);

private:
	Model &				_model;
	unsigned			_projectCount;
	std::vector<int>	_unverifiedProjectIds;
	std::vector<int>	_notRepairedProjectIds;
};

#endif
