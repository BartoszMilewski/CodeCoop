//------------------------------------
//  (c) Reliable Software, 2000 - 2009
//------------------------------------

#include "precompiled.h"
#include "FileClassifier.h"
#include "IpcConversation.h"
#include "Catalog.h"
#include "OutputSink.h"
#include "IdeContext.h"
#include "ProjectData.h"
#include "SccDll.h"
#include "FileState.h"
#include "scc.h"

#include <File/Path.h>
#include <Dbg/Assert.h>

FileListClassifier::FileListClassifier (int fileCount, char const **paths, IDEContext const * ideContext)
{
	if (ideContext != 0 && ideContext->IsProjectLocated ())
	{
		// We are running with IDE and the Code Co-op project has been
		// already determined during IDEContext creation -- add hinted project
		// to the file catalog
		std::unique_ptr<ProjectFiles> newProject (new ProjectFiles (paths,
																  ideContext->GetProjectId (),
																  ideContext->GetProjectName (),
																  ideContext->GetRootPath ()));
		_fileCatalog.push_back (std::move(newProject));
	}
	// Catalog files using projects already in the file catalog or
	// scan global project catalog.
	CatalogFiles (fileCount, paths, TheCatSingleton.GetCatalog ());
}

FileListClassifier::FileListClassifier (int projId, char const **paths)
{
	Project::Data projData;
	TheCatSingleton.GetCatalog ().GetProjectData (projId, projData);
	if (projData.IsValid ())
	{
		std::unique_ptr<ProjectFiles> newProject (new ProjectFiles (paths,
																projData.GetProjectId (),
																projData.GetProjectName (),
																projData.GetRootDir ()));
		if (paths != 0)
			newProject->AddFile (0);

		_fileCatalog.push_back (std::move(newProject));
	}
}

std::string FileListClassifier::ProjectFiles::GetFileList () const
{
	std::string fileList;
	for (Sequencer seq (*this); !seq.AtEnd (); seq.Advance ())
	{
		FilePath workPath (seq.GetFilePath ());
		if (workPath.Canonicalize (true))	// When canonicalizing path don't throw exception
		{
			workPath.ConvertToLongPath ();
			char const * path = workPath.GetDir ();
			if (FilePath::IsAbsolute (path))
			{
				fileList += "\"";
				fileList += path;
				fileList += "\" ";
			}
			else
			{
				std::string info;
				info += "Ignoring relative path '";
				info += seq.GetFilePath ();
				info += "'";
				TheOutput.Display (info.c_str ());
			}
		}
		else
		{
			std::string info;
			info += "Ignoring illegal path '";
			info += seq.GetFilePath ();
			info += "'";
			TheOutput.Display (info.c_str ());
		}
	}
	return fileList;
}

void FileListClassifier::CatalogFiles (int ideFileCount, char const **paths, Catalog & catalog)
{
	for (int ideIdx = 0; ideIdx < ideFileCount; ++ideIdx)
	{
		FilePath idePath (paths [ideIdx]);
		// Look at already cataloged projects
		typedef auto_vector<ProjectFiles>::iterator Iter;
		Iter iter = _fileCatalog.begin ();
		for (; iter != _fileCatalog.end (); ++iter)
		{
			std::string const & coopProjectRoot = (*iter)->GetProjectRootPath ();
			if (idePath.HasPrefix (coopProjectRoot))
			{
				(*iter)->AddFile (ideIdx);
				break;
			}
		}
		if (iter == _fileCatalog.end ())
		{
			// No found among already cataloged projects.
			// Search Code Co-op catalog.
			for (ProjectSeq it (catalog); !it.AtEnd (); it.Advance ())
			{
				Project::Data proj;
				it.FillInProjectData (proj);
				FilePath coopProjectRoot (proj.GetRootDir ());
				Assert (!coopProjectRoot.IsDirStrEmpty ());
				if (idePath.HasPrefix (coopProjectRoot))
				{
					// File belongs to some Code Co-op project
					Assert (!proj.GetProjectName ().empty ());
					std::unique_ptr<ProjectFiles> newProject (new ProjectFiles (paths,
																			  proj.GetProjectId (),
																			  proj.GetProjectName (),
																			  coopProjectRoot.GetDir ()));
					newProject->AddFile (ideIdx);
					_fileCatalog.push_back (std::move(newProject));
					break;
				}
			}
		}
	}
}

void FileListClassifier::ProjectFiles::FillStatus (long ideStatus [], StatusSequencer & statusSeq) const
{
	for (Sequencer projFileSeq (*this);
		 !projFileSeq.AtEnd () && !statusSeq.AtEnd ();
		 projFileSeq.Advance (), statusSeq.Advance ())
	{
		long state = SCC_STATUS_NOTCONTROLLED;
		FileState coopState (statusSeq.GetState ());
		if (!coopState.IsNone ())
		{
			state = SCC_STATUS_CONTROLLED;
			if (!coopState.IsCheckedIn ())
			{
				state |= SCC_STATUS_OUTBYUSER | SCC_STATUS_CHECKEDOUT | SCC_STATUS_MODIFIED;
			}
			if (coopState.IsCheckedOutByOthers ())
				state |= SCC_STATUS_OUTOTHER;
		}
		ideStatus [projFileSeq.GetIdeIndex ()] = state;
	}
}
