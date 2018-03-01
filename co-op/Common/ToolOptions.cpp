//------------------------------------
//  (c) Reliable Software, 2007 - 2008
//------------------------------------

#include "precompiled.h"
#include "ToolOptions.h"
#include "Registry.h"
#include "PathRegistry.h"
#include "AltDiffer.h"
#include "GlobalFileNames.h"

#include <File/Path.h>

ToolOptions::Editor::Editor ()
	: _useExternalEditor (false)
{
	Registry::UserDifferPrefs prefs;
	if (prefs.IsAlternativeEditor ())
	{
		_useExternalEditor = true;
		_externalEditorPath = prefs.GetAlternativeEditorPath ();
		_externalEditorCommand = prefs.GetAlternativeEditorCmdLine ();
	}
}

std::ostream & operator<<(std::ostream & os, ToolOptions::Editor const & editorOptions)
{
	if (editorOptions.UsesExternalEditor ())
		os << "*External editor used: " << editorOptions.GetExternalEditorPath () << " " << editorOptions.GetExternalEditorCommand () << std::endl;
	else
		os << "*Built-in Code Co-op Editor used.";
	return os;
}

ToolOptions::Differ::Differ ()
	: _useOriginalBc (false),
	  _useOurBc (false),
	  _useGuiffy (false),
	  _bcSupportsMerge(false)
{
	FilePath programPath (Registry::GetProgramPath ());
	_ourBcPath = programPath.GetFilePath (BcMergerExe);
	_originalBcPath = ::RetrieveFullBeyondComparePath (_bcSupportsMerge);
	_guiffyPath = ::GetGuiffyPath ();
	Registry::UserDifferPrefs differPrefs;
	bool isOn;
	if (differPrefs.IsAlternativeDiffer (isOn))
	{
		// Alternative differ registry keys are present
		bool useXml = false;
		std::string currentAltDiffer = differPrefs.GetAlternativeDiffer (useXml);
		if (isOn && !currentAltDiffer.empty ())
		{
			_useOriginalBc = FilePath::IsEqualDir (currentAltDiffer, _originalBcPath);
			_useOurBc = FilePath::IsEqualDir (currentAltDiffer, _ourBcPath);
			_useGuiffy = FilePath::IsEqualDir (currentAltDiffer, _guiffyPath);
		}
		// Else is not on or no path in registry -- not using alternative differ
	}
	// Else alternative differ was not setup during installation, so it is not used, but now the user can change this
}

std::ostream & operator<<(std::ostream & os, ToolOptions::Differ const & differOptions)
{
	if (differOptions.HasOriginalBc ())
		os << "Original Beyond Compare Differ: " << differOptions.GetOriginalBcPath () << std::endl;
	if (differOptions.HasBc ())
		os << "*Bundled Beyond Compare Differ: " << differOptions.GetOurBcPath () << std::endl;
	if (differOptions.HasGuiffy ())
		os << "*Guiffy Differ: " << differOptions.GetGuiffyPath () << std::endl;
	if (differOptions.UsesOriginalBc ())
		os << "*Original Beyond Compare Differ used.";
	else if (differOptions.UsesBc ())
		os << "*Beyond Compare Differ used.";
	else if (differOptions.UsesGuiffy ())
		os << "*Guiffy Differ used.";
	else
		os << "*Built-in Code Co-op Differ used.";
	return os;
}

ToolOptions::Merger::Merger ()
	: _useOriginalBc (false),
	  _useOurBc (false),
	  _useGuiffy (false),
	  _bcSupportsMerge(false)
{
	FilePath programPath (Registry::GetProgramPath ());
	_ourBcPath = programPath.GetFilePath (BcMergerExe);
	_originalBcPath = ::RetrieveFullBeyondComparePath (_bcSupportsMerge);
	_guiffyPath = ::GetGuiffyPath ();
	Registry::UserDifferPrefs mergerPrefs;
	bool isOn;
	if (mergerPrefs.IsAlternativeMerger (isOn))
	{
		// Alternative merger registry keys are present
		bool useXml = false;
		std::string currentAltMerger = mergerPrefs.GetAlternativeMerger (useXml);
		if (isOn && !currentAltMerger.empty ())
		{
			_useOriginalBc = FilePath::IsEqualDir (currentAltMerger, _originalBcPath);
			_useOurBc = FilePath::IsEqualDir (currentAltMerger, _ourBcPath);
			_useGuiffy = FilePath::IsEqualDir (currentAltMerger, _guiffyPath);
		}
		// Else is not on or no path in registry -- not using alternative differ
	}
	// Else alternative differ was not setup during installation, so it is not used, but now the user can change this
}

std::ostream & operator<<(std::ostream & os, ToolOptions::Merger const & mergerOptions)
{
	if (mergerOptions.HasOriginalBc () && mergerOptions.BcSupportsMerge())
		os << "*Beyond Compare Merger: " << mergerOptions.GetOriginalBcPath () << std::endl;
	if (mergerOptions.HasBc ())
		os << "*Bundled Beyond Compare Merger: " << mergerOptions.GetOurBcPath () << std::endl;
	if (mergerOptions.HasGuiffy ())
		os << "*Guiffy Merger: " << mergerOptions.GetGuiffyPath () << std::endl;
	if (mergerOptions.UsesOriginalBc ())
		os << "*Original Beyond Compare Merger used.";
	if (mergerOptions.UsesBc ())
		os << "*Bundled Beyond Compare Merger used.";
	else if (mergerOptions.UsesGuiffy ())
		os << "*Guiffy Merger used.";
	else
		os << "*Built-in Code Co-op Merger used.";
	return os;
}

