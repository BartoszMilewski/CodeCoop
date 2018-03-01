//----------------------------------
// (c) Reliable Software 2007 - 2008
//----------------------------------

#include "precompiled.h"
#include "MergerProxy.h"
#include "Model.h"
#include "HistoricalFiles.h"
#include "IpcExchange.h"
#include "AppInfo.h"
#include "AltDiffer.h"
#include "Registry.h"
#include "CmdLineMaker.h"
#include "PhysicalFile.h"
#include "PathFind.h"
#include "SccProxy.h"
#include "SccOptions.h"
#include "SccProxyEx.h"
#include "OutputSink.h"
#include "BuiltInMerger.h"

#include <Sys/Process.h>
#include <File/Path.h>
#include <XML/XmlTree.h>
#include <array_vector.h>

bool ExecuteXmlDifferMerger (XML::Tree & xmlArgs, 
							 bool useDefault,
							 ActiveMergerWatcher * mergerWatcher = 0,
							 GlobalId fileGid = gidInvalid);

void MergerProxy::MaterializeFolderPath (std::string const & fullTargetPath)
{
	PathFinder::MaterializeFolderPath (fullTargetPath.c_str ());
}

void MergerProxy::CopyFile (std::string const & sourceFullPath,
							std::string const & fullTargetPath,
							bool quiet)
{
	PathSplitter splitter (fullTargetPath);
	std::string targetParent (splitter.GetDrive ());
	targetParent += splitter.GetDir ();
	PathFinder::MaterializeFolderPath (targetParent.c_str (), quiet);
	File::Copy (sourceFullPath.c_str (), fullTargetPath.c_str ());
	File::MakeReadWrite (fullTargetPath);
}

LocalMergerProxy::LocalMergerProxy (Model & model)
	: _model (model)
{
}

void LocalMergerProxy::AddFile (char const * fullTargetPath, GlobalId gid, FileType type)
{
	Assert (gid != gidInvalid);
	_model.AddFile (fullTargetPath, gid, type);
}

void LocalMergerProxy::Delete (char const * fullTargetPath)
{
	_model.DeleteFile (fullTargetPath);
}

void LocalMergerProxy::ReCreateFile (char const * fullTargetPath, GlobalId gid, FileType type)
{
	Assert (gid != gidInvalid);
	Assert (_model.GetFileIndex ().FindByGid (gid) != 0);
	_model.AddFile (fullTargetPath, gid, type);	// Re-create file/folder
}

void LocalMergerProxy::MergeAttributes (std::string const & currentPath,
										std::string const & newPath,
										FileType newType)
{
	try
	{
		_model.MergeAttributes (currentPath, newPath, newType);
	}
	catch (Win::Exception ex)
	{
		TheOutput.Display (ex);
	}
	catch ( ... )
	{
		Win::ClearError ();
		TheOutput.Display ("File attribute merge -- Unknown Error", Out::Error); 
	}
}

void LocalMergerProxy::Checkout (char const * fullTargetPath, GlobalId gid)
{
	GidList files;
	files.push_back (gid);
	_model.CheckOut (files, false, false);	// Don't include folder and non-recursive
}

//-------
// Remote
//-------

RemoteMergerProxy::RemoteMergerProxy (int targetProjectId)
	: _targetProjectId (targetProjectId)
{
}

void RemoteMergerProxy::AddFile (char const * fullTargetPath, GlobalId gid, FileType type)
{
	StringArray	paths;
	paths.push_back (fullTargetPath);

	CodeCoop::Proxy proxy;
	CodeCoop::SccOptions options;
	options.SetDontCheckIn ();

	if (type.IsHeader ())
		options.SetTypeHeader ();
	else if (type.IsSource ())
		options.SetTypeSource ();
	else if (type.IsText ())
		options.SetTypeText ();
	else if (type.IsBinary ())
		options.SetTypeBinary ();

	if (!proxy.AddFile (paths.size (), paths.get (), "", options))
	{
		throw Win::InternalException ("Cannot add file at merge target.", fullTargetPath);
	}
}

void RemoteMergerProxy::Delete (char const * fullTargetPath)
{
	StringArray paths;
	paths.push_back (fullTargetPath);

	CodeCoop::Proxy proxy;
	CodeCoop::SccOptions options;

	if (!proxy.RemoveFile (paths.size (), paths.get (), options))
	{
		throw Win::InternalException ("Cannot delete file at merge target.", fullTargetPath);
	}
}

void RemoteMergerProxy::ReCreateFile (char const * fullTargetPath, GlobalId gid, FileType type)
{
	std::string cmd ("ReCreateFile target:\"");
	cmd += fullTargetPath;
	cmd += "\" gid:\"";
	cmd += ::ToHexString (gid);
	cmd += "\" type:\"";
	cmd += ::ToString (type.GetValue ());
	cmd += "\"";
	SccProxyEx proxy;
	if (!proxy.CoopCmd (_targetProjectId, cmd, false, false)) // Don't skip GUI co-op is in the project;
															  // Execute command with timeout
	{
		throw Win::InternalException ("Cannot re-create file at merge target.", fullTargetPath);
	}
}

void RemoteMergerProxy::MergeAttributes (std::string const & currentPath,
										 std::string const & newPath,
										 FileType newType)
{
	std::string cmd ("MergeAttributes currentPath:\"");
	cmd += currentPath;
	cmd += "\" newPath:\"";
	cmd += newPath;
	cmd += "\" type:\"";
	cmd += ::ToString (newType.GetValue ());
	cmd += "\"";
	SccProxyEx proxy;
	if (!proxy.CoopCmd (_targetProjectId, cmd, false, false)) // Don't skip GUI co-op is in the project;
															  // Execute command with timeout
	{
		throw Win::InternalException ("Cannot merge file attributes at merge target.", newPath.c_str ());
	}
}

void RemoteMergerProxy::Checkout (char const * fullTargetPath, GlobalId gid)
{
	StringArray paths;
	paths.push_back (fullTargetPath);

	CodeCoop::Proxy proxy;
	CodeCoop::SccOptions options;

	if (!proxy.CheckOut (paths.size (), paths.get (), options))
	{
		throw Win::InternalException ("Cannot check-out file at merge target.", fullTargetPath);
	}
}

// Global Functions

bool ExecuteXmlDifferMerger (XML::Tree & xmlArgs, 
							 bool useDefault, 
							 ActiveMergerWatcher * mergerWatcher,
							 GlobalId fileGid)
{
	XML::Node const * root = xmlArgs.GetRoot ();
	bool isAuto = (root->FindAttribValue ("auto") == "true");
	std::string toolPath;
	bool useXml = false;
	if (useDefault)
	{
		toolPath = TheAppInfo.GetDifferPath ();
	}
	else
	{
		Registry::UserDifferPrefs prefs;
		if (root->GetName () == "diff")
		{
			toolPath = prefs.GetAlternativeDiffer (useXml);
		}
		else
		{
			Assert (root->GetName () == "merge");
			if (isAuto)
				toolPath = prefs.GetAlternativeAutoMerger (useXml);
			else
				toolPath = prefs.GetAlternativeMerger (useXml);
		}
		Assert (useXml);
	}

	if (mergerWatcher != 0)
	{
		Assert (isAuto);
		Assert (fileGid != gidInvalid);
		// Start process asynchronously
		mergerWatcher->Add (xmlArgs, toolPath, fileGid);
		return true;
	}

	XmlBuf buf;
	std::ostream out (&buf); // use buf as streambuf
	xmlArgs.Write (out);
	if (out.fail ())
		throw Win::InternalException ("Differ argument string too long.");

#if 0
	std::ofstream outFile ("c:\\bin\\args.xml");
	if (outFile.fail ())
		throw Win::InternalException ("Can't open file.");
	xmlArgs.Write (outFile);
#endif

	std::ostringstream cmdLine;
	cmdLine << " /xmlspec 0x" << std::hex << buf.GetHandle ();
	if (isAuto)
	{
		Win::ChildProcess process (cmdLine.str ().c_str (), true);	// Must inherit parent's handles
		process.SetAppName (toolPath);
		process.ShowMinimizedNotActive ();
		process.Create ();
		process.WaitForDeath (10000);	// Wait 10 seconds
		if (process.IsAlive ())
			return false;

		return process.GetExitCode () == 0;
	}
	else
	{
		Win::ChildProcess process (cmdLine.str ().c_str (), true);	// Inherit parent's handles
		process.SetAppName (toolPath);
		process.ShowNormal ();
		process.Create ();
	}
	return true;
}

void ExecuteDiffer (XML::Tree & xmlArgs)
{
	bool useXml = false;
	if (UsesAltDiffer (useXml) && xmlArgs.GetRoot ()->GetName () == "diff")
	{
		if (useXml)
		{
			ExecuteXmlDifferMerger (xmlArgs, false); // non-default differ
		}
		else
		{
			// Alien differ
			AltDifferCmdLineMaker maker (xmlArgs);
			std::string cmdLine;
			std::string appName;
			maker.MakeGUICmdLine (cmdLine, appName);
			if (!cmdLine.empty ())
			{
				Win::ChildProcess differ (cmdLine.c_str (), false);	// Don't inherit parent's handles
				differ.SetAppName (appName);
				differ.ShowNormal ();
				differ.Create ();
			}
		}
	}
	else
	{
		ExecuteXmlDifferMerger (xmlArgs, true); // use our differ
	}
}

void ExecuteMerger (XML::Tree & xmlArgs)
{
	bool useXml = false;
	if (UsesAltMerger (useXml) && xmlArgs.GetRoot ()->GetName () == "merge")
	{
		if (useXml)
		{
			ExecuteXmlDifferMerger (xmlArgs, false); // non-default merger
		}
		else
		{
			// Alien merger
			AltMergerCmdLineMaker maker (xmlArgs);
			std::string cmdLine;
			std::string appName;
			maker.MakeGUICmdLine (cmdLine, appName);
			if (!cmdLine.empty ())
			{
				Win::ChildProcess differ (cmdLine.c_str (), false);	// Don't inherit parent's handles
				differ.SetAppName (appName);
				differ.ShowNormal ();
				differ.Create ();
			}
		}
	}
	else
	{
		TheOutput.Display ("No merger defined in the registry");
	}
}

// Returns true when auto merge without conflicts
bool ExecuteAutoMerger (XML::Tree & xmlArgs, 
						ActiveMergerWatcher * mergerWatcher,
						GlobalId fileGid) 
{
	Assert (xmlArgs.GetRoot ()->GetName () == "merge");
	bool useXml = false;
	if (UsesAltMerger (useXml))
	{
		if (useXml)
		{
			return ExecuteXmlDifferMerger (xmlArgs, false, mergerWatcher, fileGid);
		}
		else
		{
			AltMergerCmdLineMaker maker (xmlArgs);
			std::string cmdLine;
			std::string appPath;
			maker.MakeAutoCmdLine (cmdLine, appPath);
			if (!cmdLine.empty ())
			{
				Win::ChildProcess merger (cmdLine.c_str (), true);	// Inherit parent's handles
				merger.SetAppName (appPath);
				merger.ShowMinimizedNotActive ();
				merger.Create ();
				merger.WaitForDeath (10000);	// Wait 10 seconds
				if (merger.IsAlive ())
					return false;

				return merger.GetExitCode () == 0;
			}
		}
	}
	else
	{
		TheOutput.Display ("No auto merger defined in the registry");
	}
	return false;
}

void ExecuteEditor (XML::Tree & xmlArgs)
{
	Registry::UserDifferPrefs prefs;
	if (prefs.IsAlternativeEditor ())
	{
		AltEditorCmdLineMaker maker (xmlArgs);
		std::string cmdLine;
		std::string appName;
		maker.MakeGUICmdLine (cmdLine, appName);
		if (!cmdLine.empty ())
		{
			Win::ChildProcess editor (cmdLine.c_str (), false);	// Don't inherit parent's handles
			editor.SetAppName (appName);
			editor.ShowNormal ();
			editor.Create ();
		}
		return;
	}

	ExecuteXmlDifferMerger (xmlArgs, true); // use default differ
}

