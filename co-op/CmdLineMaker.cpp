//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

#include "precompiled.h"
#include "CmdLineMaker.h"
#include "PhysicalFile.h"
#include "Registry.h"
#include "OutputSink.h"
#include <XML/XmlTree.h>

bool CmdLineMaker::HasValue (std::string const & key) const
{
	for (ArgIter it = _args.begin (); it != _args.end (); ++it)
		if (it->first == key)
			return true;
	return false;
}

void CmdLineMaker::TranslateArgs (std::string & cmdLine)
{
	for (ArgIter iter = _args.begin (); iter != _args.end (); ++iter)
	{
		std::pair<std::string, std::string> const & arg = *iter;
		unsigned begin = 0;
		for (;;) // multiple occurrences
		{
			begin = cmdLine.find (arg.first, begin);
			if (begin == std::string::npos)
				break;
			cmdLine.replace (begin, arg.first.length (), arg.second);
		}
	}
}

AltEditorCmdLineMaker::AltEditorCmdLineMaker (XML::Tree const & xmlArgs)
{
	XML::Node const * root = xmlArgs.GetRoot ();
	Assert (root->GetName () == "edit");
	XML::Node const * child = root->FindChildByAttrib ("file", "role", "current");
	Assert (child != 0);
	_args.push_back (std::make_pair ("$file1", child->GetAttribValue ("path")));
}

void AltEditorCmdLineMaker::MakeGUICmdLine (std::string & cmdLine, std::string & appPath)
{
	Registry::UserDifferPrefs prefs;
	appPath = prefs.GetAlternativeEditorPath ();
	std::string cmdLineTemplate = prefs.GetAlternativeEditorCmdLine ();
	if (appPath.empty () || cmdLineTemplate.empty ())
	{
		TheOutput.Display ("Cannot find registry entries for the alternative editor.\n"
						   "Please, execute Tools>Editor menu command again.");
		return;
	}

	cmdLine += " ";
	cmdLine += cmdLineTemplate;

	TranslateArgs (cmdLine);
}

AltDifferCmdLineMaker::AltDifferCmdLineMaker (XML::Tree const & xmlArgs)
{
	XML::Node const * root = xmlArgs.GetRoot ();
	Assert (root->GetName () == "diff");
	// Before file
	XML::Node const * child = root->FindChildByAttrib ("file", "role", "before");
	Assert (child != 0);
	_args.push_back (std::make_pair ("$file1", child->GetAttribValue ("path")));
	std::string const & dispPath = child->FindAttribValue ("display-path");
	if (!dispPath.empty ())
		_args.push_back (std::make_pair ("$title1", dispPath));
	// After file
	XML::Node const * afterChild = root->FindChildByAttrib ("file", "role", "after");
	XML::Node const * currentChild = root->FindChildByAttrib ("file", "role", "current");
	if (afterChild != 0)
	{
		_args.push_back (std::make_pair ("$file2", afterChild->GetAttribValue ("path")));
		std::string const & dispPath = afterChild->FindAttribValue ("display-path");
		if (!dispPath.empty ())
			_args.push_back (std::make_pair ("$title2", dispPath));
		if (currentChild != 0)
			_args.push_back (std::make_pair ("$file3", currentChild->GetAttribValue ("path")));
	}
	else
	{
		Assert (currentChild != 0);
		_args.push_back (std::make_pair ("$file2", currentChild->GetAttribValue ("path")));
	}
}

void AltDifferCmdLineMaker::MakeGUICmdLine (std::string & cmdLine, std::string & appPath)
{
	Registry::UserDifferPrefs prefs;
	bool useXml = false;
	appPath = prefs.GetAlternativeDiffer (useXml);
	Assert (!useXml);
	std::string cmdLineTemplate = prefs.GetDifferCmdLine (HasValue ("$title2"));
	if (appPath.empty () || cmdLineTemplate.empty ())
	{
		TheOutput.Display ("Cannot find registry entries for the alternative differ.\n"
						   "Please, execute Tools>Differ menu command again.");
		return;
	}

	cmdLine += " ";
	cmdLine += cmdLineTemplate;

	TranslateArgs (cmdLine);
}


AltMergerCmdLineMaker::AltMergerCmdLineMaker (PhysicalFile const & file)
{
	_args.push_back (std::make_pair ("$file1", file.GetFullPath (Area::Project)));
	_args.push_back (std::make_pair ("$file2", file.GetFullPath (Area::Synch)));
	_args.push_back (std::make_pair ("$file3", file.GetFullPath (Area::Reference)));
	_args.push_back (std::make_pair ("$file4", file.GetFullPath (Area::Project)));
}

AltMergerCmdLineMaker::AltMergerCmdLineMaker (XML::Tree const & xmlArgs)
{
	XML::Node const * root = xmlArgs.GetRoot ();
	Assert (root->GetName () == "merge");
	for (XML::Node::ConstChildIter it = root->FirstChild (); it != root->LastChild (); ++it)
	{
		XML::Node const * node = *it;
		Assert (node->GetName () == "file");
		std::string const & role = node->GetAttribValue ("role");
		std::string const & path = node->GetAttribValue ("path");
		std::string const & title = node->FindAttribValue ("display-path");
		if (role == "source")
		{
			_args.push_back (std::make_pair ("$file1", path));
			_args.push_back (std::make_pair ("$title1", title));
		}
		else if (role == "target")
		{
			_args.push_back (std::make_pair ("$file2", path));
			_args.push_back (std::make_pair ("$title2", title));
		}
		else if (role == "reference")
		{
			_args.push_back (std::make_pair ("$file3", path));
			_args.push_back (std::make_pair ("$title3", title));
		}
		else if (role == "result")
		{
			_args.push_back (std::make_pair ("$file4", path));
		}
	}
}

void AltMergerCmdLineMaker::MakeGUICmdLine (std::string & cmdLine, std::string & appPath)
{
	Registry::UserDifferPrefs prefs;
	bool useXml = false;
	appPath = prefs.GetAlternativeMerger (useXml);
	Assert (!useXml);
	std::string cmdLineTemplate = prefs.GetMergerCmdLine ();
	if (appPath.empty () || cmdLineTemplate.empty ())
	{
		TheOutput.Display ("Cannot find registry entries for the alternative merger.\n"
						   "Please, execute Tools>Merger menu command again.");
		return;
	}

	cmdLine += " ";
	cmdLine += cmdLineTemplate;

	TranslateArgs (cmdLine);
}

void AltMergerCmdLineMaker::MakeAutoCmdLine (std::string & cmdLine, std::string & appPath)
{
	Registry::UserDifferPrefs prefs;
	bool useXml = false;
	appPath = prefs.GetAlternativeAutoMerger (useXml);
	Assert (!useXml);
	std::string cmdLineTemplate = prefs.GetAutoMergerCmdLine ();

	if (appPath.empty () || cmdLineTemplate.empty ())
	{
		_args.push_back (std::make_pair ("$title1", "Target Branch"));
		_args.push_back (std::make_pair ("$title2", "Source Branch"));
		_args.push_back (std::make_pair ("$title3", "Common Version"));
		MakeGUICmdLine (cmdLine, appPath);
		return;
	}

	cmdLine += " ";
	cmdLine += cmdLineTemplate;

	TranslateArgs (cmdLine);
}
