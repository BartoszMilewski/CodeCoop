// ----------------------------------
// (c) Reliable Software, 2004 - 2006
// ----------------------------------

#include "precompiled.h"
#include "AltDiffer.h"
#include "OutputSink.h"
#include "Registry.h"

#include <File/Path.h>
#include <File/File.h>
#include <StringOp.h>

bool IsAtLeastVersion (std::string & version, int major, int minor)
{
	unsigned pos = version.find ('.');

	int majorVersion = ToInt (version.substr (0, pos));
	if (majorVersion < major)
		return false;

	if (pos == std::string::npos)
		return minor == 0;

	++pos;
	if (majorVersion == major)
	{
		unsigned pos2 = version.find ('.', pos);
		int minorVersion = ToInt (version.substr (pos, pos2));
		if (minorVersion < minor)
			return false;
	}
	return true;
}

std::string RetrieveFullBeyondComparePath (bool & supportsMerge)
{
	supportsMerge = false;
	std::string exePath;
	RegKey::Check bcKey (RegKey::CurrentUser (), "Software\\Scooter Software\\Beyond Compare 3");
	if (bcKey.Exists ())
	{
		std::string version = bcKey.GetStringVal ("Version");
		if (IsAtLeastVersion (version, 3, 0))
		{
			unsigned long isPro;
			if (bcKey.GetValueLong ("SupportsMerge", isPro))
				supportsMerge = (isPro == 1);
			
			exePath = bcKey.GetStringVal ("ExePath");
		}
	}
	return exePath;
}

std::string GetGuiffyPath ()
{
	std::string exePath;
	RegKey::Check guiffyKey (RegKey::LocalMachine (), "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Guiffy");
	if (guiffyKey.Exists ())
	{
		FilePath guiffyPath (guiffyKey.GetStringVal ("InstallLocation"));
		exePath = guiffyPath.GetFilePath ("guiffy.exe");
	}
	return exePath;
}

std::string GetAraxisPath ()
{
	// REVISIT: return path only if professional version installed
	// because only professional version supports 3-way merge in command line mode.
	// Waiting for the information from ARAXIS how to detect professional version.
	std::string exePath;
#if 0
	// REVISIT: to fully utilise Araxis we have to use automation.
	RegKey::Check araxisKey (RegKey::LocalMachine (), "SOFTWARE\\Classes\\CLSID\\{6bc05a94-8ec8-11d2-b346-0000e835aa2c}\\LocalServer32");
	if (araxisKey.Exists ())
	{
		exePath = araxisKey.GetStringVal ("");
		// Remove quotes from the path string
		size_t pos = exePath.find ('"');
		if (pos != std::string::npos)
			exePath.erase (pos, 1);
		pos = exePath.find ('"');
		if (pos != std::string::npos)
			exePath.erase (pos, 1);
	}
#endif
	return exePath;
}

// Used in co-op

bool UsesAltDiffer (bool & useXml)
{
	Registry::UserDifferPrefs prefs;
	bool isOn;
	if (prefs.IsAlternativeDiffer (isOn))
	{
		// is in registry, but is it on, and is the path non-empty?
		if (isOn)
		{
			std::string altPath = prefs.GetAlternativeDiffer (useXml);
			if (altPath.empty ())
				return false;
			if (useXml)
				return true;
			// Alien differ
			if (prefs.GetDifferCmdLine (false).empty ())
			{
				// It must be BC, set command line
				if (altPath.find ("BCDiffer") != std::string::npos
					|| altPath.find ("BCMerge") != std::string::npos
					|| altPath.find ("Beyond Compare") != std::string::npos)
				{
					prefs.SetAlternativeDiffer (true, altPath, BCDIFFER_CMDLINE, BCDIFFER_CMDLINE2, true);
				}
			}
			return true;
		}
		else
			return false;
	}
	// not in our registry, look for Beyond Compare
	bool bcSupportsMerge = false;
	std::string exePath = ::RetrieveFullBeyondComparePath (bcSupportsMerge);
	if (exePath.empty ())
		return false;

	// we have the right version of Beyond Compare
	// we don't care about merge support
	std::string msg ("Code Co-op detected Beyond Compare 3 on your computer\n");
	msg += "Would you like to use Beyond Compare to display file differences?\n";
	msg += "(You can change this setting later in Program>Tools>Differ menu.)";
	Out::Answer answer = TheOutput.Prompt (msg.c_str ());
	if (answer == Out::Cancel)
		return false;

	prefs.SetAlternativeDiffer (answer == Out::Yes, exePath, BCDIFFER_CMDLINE, BCDIFFER_CMDLINE2, true);
	return answer == Out::Yes;
}

void SetUpOurAltDiffer (std::string const & ourBcPath, bool quiet)
{
	Registry::UserDifferPrefs prefs;
	bool isOn;
	bool useXml = false;
	// is alt differ registry set?
	if (!prefs.IsAlternativeDiffer (isOn) || prefs.GetAlternativeDiffer (useXml).empty ())
	{
		Out::Answer answer = Out::Yes;
		if (!quiet)
		{
			std::string msg ("Would you like to use Beyond Compare to display file differences?\n");
			msg += "(You can change this setting later in Program.Options menu.)";
			answer = TheOutput.Prompt (msg.c_str ());
			if (answer == Out::Cancel)
				return;
		}
		prefs.SetAlternativeDiffer (answer == Out::Yes, ourBcPath, BCDIFFER_CMDLINE, BCDIFFER_CMDLINE2, true);
	}
}

void SetupGuiffyRegistry (bool isOn, std::string const & exePath)
{
	Registry::UserDifferPrefs prefs;
	prefs.SetAlternativeMerger (isOn, exePath, GUIFFYMERGER_CMDLINE, false);
	PathSplitter splitter (exePath);
	std::string path (splitter.GetDrive ());
	path += splitter.GetDir ();
	FilePath guiffyPath (path);
	char const * autoMergerPath = guiffyPath.GetFilePath ("suremerge.exe");
	if (File::Exists (autoMergerPath))
		prefs.SetAlternativeAutoMerger (autoMergerPath, GUIFFYAUTOMERGER_CMDLINE, false);
}

void SetupAraxisRegistry (bool isOn, std::string const & exePath)
{
	PathSplitter splitter (exePath);
	std::string path (splitter.GetDrive ());
	path += splitter.GetDir ();
	FilePath araxisPath (path);
	char const * mergerPath = araxisPath.GetFilePath ("compare.exe");
	if (File::Exists (mergerPath))
	{
		Registry::UserDifferPrefs prefs;
		prefs.SetAlternativeMerger (isOn, mergerPath, ARAXISMERGER_CMDLINE, false);
		prefs.SetAlternativeAutoMerger (mergerPath, ARAXISAUTOMERGER_CMDLINE, false);
	}
}

bool UsesAltMerger (bool & useXml)
{
	Registry::UserDifferPrefs prefs;
	bool isOn;
	if (prefs.IsAlternativeMerger (isOn))
	{
		// is in registry, but is it on, and is the path non-empty?
		if (isOn)
		{
			std::string altPath = prefs.GetAlternativeMerger (useXml);
			if (altPath.empty ())
				return false;
			if (useXml)
				return true;
			// Alien merger
			if (prefs.GetMergerCmdLine ().empty ())
			{
				// Set command line
				if (altPath.find ("guiffy") != std::string::npos)
				{
					// Guiffy merger command line
					SetupGuiffyRegistry (true, altPath);
				}
				else if (altPath.find ("BCDiffer") != std::string::npos ||
						 altPath.find ("BCMerge") != std::string::npos ||
						 altPath.find ("Beyond Compare") != std::string::npos)
				{
					prefs.SetAlternativeDiffer (true, altPath, BCDIFFER_CMDLINE, BCDIFFER_CMDLINE2, true);
				}
			}
			return true;
		}
		else
			return false;
	}
	// not in registry, look for Guiffy
	std::string exePath = ::GetGuiffyPath ();
	if (!exePath.empty ())
	{
		// we have the Guiffy merger
		std::string msg ("Code Co-op detected Guiffy on your computer.\n");
		msg += "Would you like to use Guiffy to merger file differences?\n";
		msg += "(You can change this setting later in Program>Tools>Merger menu.)";
		Out::Answer answer = TheOutput.Prompt (msg.c_str ());
		if (answer == Out::Cancel)
			return false;

		SetupGuiffyRegistry (answer == Out::Yes, exePath);
		return answer == Out::Yes;
	}

	bool bcSupportsMerge = false;
	exePath = ::RetrieveFullBeyondComparePath (bcSupportsMerge);
	if (!exePath.empty () && bcSupportsMerge)
	{
		// We have the Beyond Compare merger.
		std::string msg ("Code Co-op detected Beyond Compare 3 Pro on your computer.\n");
		msg += "Would you like to use Beyond Compare to merge file differences?\n";
		msg += "(You can change this setting later in Program>Tools>Merger menu.)";
		Out::Answer answer = TheOutput.Prompt (msg.c_str ());
		if (answer == Out::Cancel)
			return false;

		prefs.SetAlternativeMerger (answer == Out::Yes, exePath, BCMERGER_CMDLINE, true);
		prefs.SetAlternativeAutoMerger (exePath, BCAUTOMERGER_CMDLINE, true);
		return answer == Out::Yes;
	}

	// Look for Araxis.
	exePath = ::GetAraxisPath ();
	if (!exePath.empty ())
	{
		// We have the Araxis merger.
		std::string msg ("Code Co-op detected Araxis on your computer.\n");
		msg += "Would you like to use Araxis to merger file differences?\n";
		msg += "(You can change this setting later in Program>Tools>Merger menu.)";
		Out::Answer answer = TheOutput.Prompt (msg.c_str ());
		if (answer == Out::Cancel)
			return false;

		SetupAraxisRegistry (answer == Out::Yes, exePath);
		return answer == Out::Yes;
	}

	return false;
}
