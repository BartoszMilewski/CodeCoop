//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "precompiled.h"
#include "ProjectPathVerifier.h"
#include "Catalog.h"
#include "PathFind.h"
#include "ProjectPathClassifier.h"
#include "Prompter.h"
#include "OutputSink.h"
#include "SelectRootPathDlg.h"

#include <File/File.h>
#include <Ctrl/ProgressMeter.h>

Project::RootPathVerifier::RootPathVerifier (Catalog & catalog)
	: _catalog (catalog),
	  _projectCount (0)
{
	for (ProjectSeq seq (_catalog); !seq.AtEnd (); seq.Advance ())
		_projectCount++;
}

bool Project::RootPathVerifier::Verify (Progress::Meter & meter, bool skipUnavailable)
{
	_projectsWithoutRootPath.clear ();
	meter.SetActivity ("Checking project root paths.");
	meter.SetRange (0, _projectCount, 1);
	for (ProjectSeq seq (_catalog); !seq.AtEnd (); seq.Advance ())
	{
		meter.StepAndCheck ();

		if (skipUnavailable && seq.IsProjectUnavailable ())
			continue;	// Skip projects marked as unavailable

		bool success = true;
		Project::Data projectData;
		seq.FillInProjectData (projectData);
		if (File::Exists (projectData.GetRootDir ()))
			continue;	// Skip projects with existing root paths

		// Try materializing project root path
		meter.SetActivity (projectData.GetRootDir ());
		try
		{
			PathFinder::MaterializeFolderPath (projectData.GetRootDir (), true);	// Quiet
		}
		catch ( ... )
		{
			success = false;
		}
		if (!success)
		{
			_projectsWithoutRootPath.push_back (projectData);
		}
	}
	return _projectsWithoutRootPath.empty ();
}

void Project::RootPathVerifier::PromptForNew ()
{
	Assert (!_projectsWithoutRootPath.empty ());
	Project::PathClassifier pathClassifier (_projectsWithoutRootPath);
	Project::PathClassifier::Sequencer seq (pathClassifier);
	// Iterate over groups with the same path prefix
	while (!seq.AtEnd ())
	{
		do
		{
			// Get project sequencer for this group of projects
			Project::PathClassifier::ProjectSequencer projSeq = seq.GetProjectSequencer ();
			SelectRootData dlgData (projSeq);
			SelectRootCtrl dlgCtrl (dlgData);
			if (ThePrompter.GetData (dlgCtrl))
			{
				// User OK'ed root path selection dialog.
				// Change projects root paths in the catalog.
				Assert (!dlgData.GetNewPrefix ().empty ());
				for (; !projSeq.AtEnd (); projSeq.Advance ())
				{
					FilePath newPath (dlgData.GetNewPrefix ());
					for (FullPathSeq pathSeq (projSeq.GetRootPath ()); !pathSeq.AtEnd (); pathSeq.Advance ())
						newPath.DirDown (pathSeq.GetSegment ().c_str ());

					_catalog.MoveProjectTree (projSeq.GetProjectId (), newPath);
				}
			}
			else
			{
				// User canceled root path selection dialog.
				Out::Answer userChoice =
					TheOutput.Prompt ("If you don't provide a  new root path for a project, it will be marked as \"unavailable\".\n"
									  "You can visit such project later and correct the situation.\n\n"
									  "Are you sure you want to proceed?", Out::PromptStyle (Out::YesNo, Out::No, Out::Question));

				if (userChoice == Out::No)
					continue; // Try the dialog again

				// Mark all project as unavailable.
				for (; !projSeq.AtEnd (); projSeq.Advance ())
				{
					_catalog.MarkProjectUnavailable (projSeq.GetProjectId ());
				}
			}
		} while (false);

		seq.Advance ();
	}
}

