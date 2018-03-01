//----------------------------------
// (c) Reliable Software 2000 - 2008
//----------------------------------

#include "precompiled.h"
#include "Registry.h"
#include "MemberInfo.h"

#include <Graph/Font.h> // Win::FontMaker

bool Registry::CanWriteToMachineRegKey ()
{
	try
	{
		RegKey::LocalMachine keyMachine;
		RegKey::Existing keySoftware (keyMachine, "Software");
		RegKey::New keyReliSoft (keySoftware, "Reliable Software");
		RegKey::New keyCoop (keyReliSoft, "Code Co-op");

		bool success = keyCoop.SetValueString ("Test", "test", true);
		if (success)
			keyCoop.DeleteValue ("Test");
		return success;
	}
	catch ( ... )
	{}
	return false;
}

Registry::RecentProjectList::RecentProjectList ()
{
	Registry::CoopUser keyState ("State");
	MultiString projectIds;
	keyState.Key ().GetMultiString ("RecentProjectIds", projectIds);
	unsigned int idx = 0;
	for (MultiString::const_iterator it = projectIds.begin ();
		 (it != projectIds.end ()) && (idx < RecentProjectCount);
		 ++it)
	{
		int id = ToInt (*it);
		if (id > 0)
		{
			_ids.push_back (id);
			++idx;
		}
	}
}

void Registry::RecentProjectList::Add (unsigned int id)
{
	Assert (id > 0);
	
	unsigned int currentSize = _ids.size ();
	std::vector<unsigned int>::const_iterator idToBeAdded = std::find (_ids.begin (), _ids.end (), id);
	if (idToBeAdded == _ids.end ())
	{
		if (currentSize < RecentProjectCount)
		{
			_ids.resize (currentSize + 1, id);
		}
		else
		{
			// "shift left" by 1 position
			std::copy (_ids.begin () + 1, _ids.end (), _ids.begin ());
			_ids [_ids.size () - 1] = id;
		}
	}
	else
	{
		unsigned int currentPos = idToBeAdded - _ids.begin ();
		std::copy (_ids.begin () + currentPos + 1, _ids.end (), _ids.begin () + currentPos);
		_ids [currentSize - 1] = id;			
	}

	MultiString projectIds;
	for (unsigned int i = 0; i < _ids.size (); ++i)
		projectIds.push_back (ToString (_ids [i]));

	Registry::CoopUser keyState ("State");
	keyState.Key ().SetValueMultiString ("RecentProjectIds", projectIds);
}

void Registry::SetRecentProject ( char const * projDir)
{
	Registry::CoopUser keyState ("State");
	keyState.Key ().SetValueString ("RecentProjectDir", projDir);
}

std::string Registry::GetRecentProject ()
{
	Registry::CoopUser keyState ("State");
	return keyState.Key ().GetStringVal ("RecentProjectDir");
}

void Registry::StoreUserDescription (MemberDescription const & user)
{
	Registry::CoopUser thisUser ("User");
	thisUser.Key ().SetValueString ("Name", user.GetName ().c_str ());
	thisUser.Key ().SetValueString ("Email", user.GetHubId ().c_str ());
	thisUser.Key ().SetValueString ("Phone", user.GetComment ().c_str ());
}

void Registry::ReadUserDescription (MemberDescription & user)
{
	Registry::UserInfo key;
	user.SetName (key.GetName ().c_str ());
	user.SetHubId (key.GetEmail ().c_str ());
	user.SetComment (key.GetPhone ().c_str ());
}

void Registry::SetNagging (bool flag)
{
	Registry::CoopUser thisUser ("User");
	thisUser.Key ().SetValueLong ("Nagging", flag ? 0 : 1);
}

bool Registry::IsNagging ()
{
	Registry::CoopUser thisUser ("User");
	unsigned long value;
	if (thisUser.Key ().GetValueLong ("Nagging", value))
	{
		// Value present -- 0 --> nag; 1 --> don't nag
		return value == 0;
	}
	// Value not present -- nag
	return true;
}

void Registry::BeginnerMessageOff (int msgId)
{
	std::string msgName = ToString (msgId);
	Registry::Preferences settings;
	RegKey::Existing beginnerMode (settings.Key (), "Beginner Mode");
	beginnerMode.SetValueLong (msgName.c_str (), 0);
}

bool Registry::IsBeginnerMessageOff (int msgId)
{
	std::string msgName = ToString (msgId);
	Registry::Preferences settings;
	RegKey::Existing beginnerMode (settings.Key (), "Beginner Mode");
	unsigned long msgStatus;
	// don't care about the value
	return beginnerMode.GetValueLong (msgName.c_str (), msgStatus);
}

bool Registry::IsBeginnerModeOn ()
{
	// Check in the registry if beginner's mode is on
	Registry::Preferences settings;
	RegKey::New beginnerMode (settings.Key (), "Beginner Mode");
	unsigned long modeStatus;
	if (beginnerMode.GetValueLong ("Status", modeStatus))
	{
		return (modeStatus == 1);
	}
	else
	{
		// Status not stored in registry -- assume beginner mode is on
		return true;
	}
}

int Registry::CountDisabledBeginner (int begin, int end)
{
	Registry::Preferences settings;
	RegKey::New beginnerMode (settings.Key (), "Beginner Mode");
	int count = 0;
	// Count the disabled beginner help messages
	for (int i = begin; i < end; ++i)
	{
		std::string msgName = ToString (i);
		unsigned long msgStatus;
		if (beginnerMode.GetValueLong (msgName.c_str (), msgStatus))
		{
			count++;
		}
	}
	return count;
}

void Registry::SetBeginnerMode (bool on)
{
	Registry::Preferences settings;
	// Set Beginner Mode status
	RegKey::Existing beginnerMode (settings.Key (), "Beginner Mode");
	unsigned long modeStatus = on ? 1 : 0;
	beginnerMode.SetValueLong ("Status", modeStatus);
	if (on)
	{
		// Delete all disabled messages
		std::vector<std::string> valueToDelete;
		for (RegKey::ValueSeq valueSeq (beginnerMode.ToNative ()); !valueSeq.AtEnd (); valueSeq.Advance ())
		{
			std::string valueName (valueSeq.GetName ());
			if (!IsNocaseEqual (valueName, "Status"))
				valueToDelete.push_back (valueName);
		}
		typedef std::vector<std::string>::const_iterator StrSeq;
		for (StrSeq seq = valueToDelete.begin (); seq != valueToDelete.end (); ++seq)
		{
			std::string const & valueName = *seq;
			beginnerMode.DeleteValue (valueName);
		}
	}
}

bool Registry::IsFirstRun ()
{
	Registry::DispatcherUserRoot dispatcher;
	return !dispatcher.Key ().IsValuePresent ("ConfigurationState");
}

bool Registry::IsRestoredConfiguration ()
{
	Registry::DispatcherUserRoot dispatcher;
	std::string configState = dispatcher.Key ().GetStringVal ("ConfigurationState");
	return IsNocaseEqual (configState, "Restored");
}

void Registry::UserPreferences::GetWinPlacement (std::string const & keyName,
												 Win::Placement & placement) const
{
	RegKey::New placementKey (_pref.Key (), keyName.c_str ());
	RegKey::ReadWinPlacement (placement, placementKey);
	Validate (placement);
}

void Registry::UserPreferences::GetListWinPlacement (Win::Placement & placement)
{
	GetWinPlacement ("List Window", placement);
}

std::string Registry::UserPreferences::GetFilePath (std::string const & pathName)
{
	return GetStringValue ("File Paths", pathName);
}

std::string Registry::UserPreferences::GetOption (std::string const & optionName)
{
	return GetStringValue ("Options", optionName);
}

std::string Registry::UserPreferences::GetOptionDecrypt (std::string const & optionName)
{
	return GetStringValueDecrypt ("Options", optionName);
}

void Registry::UserPreferences::SaveWinPlacement (std::string const & keyName,
												  Win::Placement & placement)
{
	RegKey::New placementKey (_pref.Key (), keyName.c_str ());
	RegKey::SaveWinPlacement (placement, placementKey);
}

void Registry::UserPreferences::SaveListWinPlacement (Win::Placement & placement)
{
	SaveWinPlacement ("List Window", placement);
}

void Registry::UserPreferences::SaveFilePath (std::string const & pathName,
											  std::string const & path)
{
	SaveStringValue ("File Paths", pathName, path);
}

void Registry::UserPreferences::SaveOption (std::string const & optionName,
											std::string const & option)
{
	SaveStringValue ("Options", optionName, option);
}

void Registry::UserPreferences::SaveOptionEncrypt (std::string const & optionName,
												   std::string const & option)
{
	SaveStringValueEncrypt ("Options", optionName, option);
}

bool Registry::UserPreferences::CanAsk (unsigned int promptId)
{
	RegKey::New promptsKey (_pref.Key (), "Prompts");
	unsigned long value;
	if (promptsKey.GetValueLong (ToString (promptId).c_str (), value))
	{
		return value == 1;
	}
	return true;
}

void Registry::UserPreferences::TurnOffPrompt (unsigned int promptId)
{
	RegKey::New promptsKey (_pref.Key (), "Prompts");
	promptsKey.SetValueLong (ToString (promptId).c_str (), 0);
};

void Registry::UserPreferences::Validate (Win::Placement & placement) const
{
	Win::Rect rect;
	placement.GetRect (rect);
	if (rect.Width () <= 0 || rect.Height () <= 0)
	{
		// Placement not found in the registry or corrupted
		Win::Rect initRect (80, 80, 800, 800);
		placement.SetRect (initRect);
		placement.SetNormal ();
	}
}

void Registry::UserPreferences::SaveStringValue (std::string const & keyName,
												 std::string const & valueName,
												 std::string const & value)
{
	RegKey::New regKey (_pref.Key (), keyName);
	regKey.SetValueString (valueName, value);
}

std::string Registry::UserPreferences::GetStringValue (std::string const & keyName,
													   std::string const & valueName)
{
	RegKey::New pathKey (_pref.Key (), keyName);
	return pathKey.GetStringVal (valueName);
}

void Registry::UserPreferences::SaveStringValueEncrypt (std::string const & keyName,
														std::string const & valueName,
														std::string const & value)
{
	RegKey::New regKey (_pref.Key (), keyName);
	regKey.SetStringEncrypt (valueName, value,  true); // Quiet
}

std::string Registry::UserPreferences::GetStringValueDecrypt (std::string const & keyName,
															  std::string const & valueName)
{
	RegKey::New regKey (_pref.Key (), keyName);
	std::string buf;
	regKey.GetStringDecrypt (valueName, buf);
	return buf;
}


bool Registry::UserDispatcherPrefs::IsInitialized ()
{
	DispatcherPrefTest pref;
	return pref.Exists ();
}

// Alternative Differ

bool Registry::UserDifferPrefs::IsAlternativeDiffer (bool & isOn)
{
	unsigned long value;
	bool result = _alternativeDiff.Key ().GetValueLong ("On", value);
	if (result)
	{
		isOn = (value != 0);
		return true;
	}
	else
		return false;
}

bool Registry::UserDifferPrefs::IsAlternativeDifferValid ()
{
	bool useXml = false;
	std::string differPath = GetAlternativeDiffer (useXml);
	std::string differCmdLine = GetDifferCmdLine (false);
	return !differPath.empty () &&
		   File::Exists (differPath.c_str ()) &&
		   (useXml || !differCmdLine.empty ());
}

std::string Registry::UserDifferPrefs::GetAlternativeDiffer (bool & useXml)
{
	unsigned long value;
	if (_alternativeDiff.Key ().GetValueLong ("UseXml", value))
		useXml = (value != 0);
	return _alternativeDiff.Key ().GetStringVal ("Path");
}

void Registry::UserDifferPrefs::SetAlternativeDiffer (bool isOn, 
													  std::string const & exePath, 
													  std::string const & cmdLine1,
													  std::string const & cmdLine2,
													  bool useXml)
{
	_alternativeDiff.Key ().SetValueString ("Path", exePath);
	_alternativeDiff.Key ().SetValueString ("CmdLine", cmdLine1);
	if (!cmdLine2.empty ())
		_alternativeDiff.Key ().SetValueString ("CmdLine2", cmdLine2);
	_alternativeDiff.Key ().SetValueLong ("On", isOn? 1: 0);
	_alternativeDiff.Key ().SetValueLong ("UseXml", useXml? 1: 0);
}

void Registry::UserDifferPrefs::ToggleAlternativeDiffer (bool isOn)
{
	_alternativeDiff.Key ().SetValueLong ("On", isOn? 1: 0);
}

std::string Registry::UserDifferPrefs::GetDifferCmdLine (bool twoArg)
{
	std::string cmdLine;
	if (twoArg)
		cmdLine = _alternativeDiff.Key ().GetStringVal ("CmdLine2");
	if (cmdLine.empty ())
		cmdLine = _alternativeDiff.Key ().GetStringVal ("CmdLine");
	return cmdLine;
}

// Alternative Merger

bool Registry::UserDifferPrefs::IsAlternativeMerger (bool & isOn)
{
	unsigned long value;
	bool result = _alternativeMerge.Key ().GetValueLong ("On", value);
	if (result)
	{
		isOn = (value != 0);
		return true;
	}
	else
		return false;
}

bool Registry::UserDifferPrefs::IsAlternativeMergerValid ()
{
	bool useXml = false;
	std::string mergerPath = GetAlternativeMerger (useXml);
	std::string mergerCmdLine = GetMergerCmdLine ();
	bool mergerExists = !mergerPath.empty () && File::Exists (mergerPath.c_str ());
	if (!mergerExists || mergerCmdLine.empty ())
		return false;
	else if (useXml)
		return true;
	// we have an alien merger, check the alien auto merger
	useXml = false;
	std::string autoMergerPath = GetAlternativeAutoMerger (useXml);
	Assert (!useXml);
	std::string autoMergerCmdLine = GetAutoMergerCmdLine ();
	return !autoMergerPath.empty () 
			&& File::Exists (autoMergerPath.c_str ()) 
			&& !autoMergerCmdLine.empty ();
}

std::string Registry::UserDifferPrefs::GetAlternativeMerger (bool & useXml)
{
	unsigned long value;
	if (_alternativeMerge.Key ().GetValueLong ("UseXml", value))
		useXml = (value != 0);
	return _alternativeMerge.Key ().GetStringVal ("Path");
}

std::string Registry::UserDifferPrefs::GetAlternativeAutoMerger (bool & useXml)
{
	// if useXml is true, use the same merger path
	unsigned long value;
	if (_alternativeMerge.Key ().GetValueLong ("UseXml", value))
	{
		useXml = (value != 0);
		if (useXml)
			return _alternativeMerge.Key ().GetStringVal ("Path");
	}
	return _alternativeMerge.Key ().GetStringVal ("AutoPath");
}

void Registry::UserDifferPrefs::SetAlternativeMerger (bool isOn, 
													  std::string const & exePath, 
													  std::string const & cmdLine,
													  bool useXml)
{
	_alternativeMerge.Key ().SetValueString ("Path", exePath);
	_alternativeMerge.Key ().SetValueString ("CmdLine", cmdLine);
	_alternativeMerge.Key ().SetValueLong ("On", isOn? 1: 0);
	_alternativeMerge.Key ().SetValueLong ("UseXml", useXml? 1: 0);
}

void Registry::UserDifferPrefs::SetAlternativeAutoMerger (std::string const & exePath, 
														  std::string const & cmdLine,
														  bool useXml)
{
	_alternativeMerge.Key ().SetValueString ("AutoPath", exePath);
	_alternativeMerge.Key ().SetValueString ("AutoCmdLine", cmdLine);
	_alternativeDiff.Key ().SetValueLong ("UseXml", useXml? 1: 0);
}

void Registry::UserDifferPrefs::ToggleAlternativeMerger (bool isOn)
{
	_alternativeMerge.Key ().SetValueLong ("On", isOn? 1: 0);
}

std::string Registry::UserDifferPrefs::GetMergerCmdLine ()
{
	return _alternativeMerge.Key ().GetStringVal ("CmdLine");
}

std::string Registry::UserDifferPrefs::GetAutoMergerCmdLine ()
{
	return _alternativeMerge.Key ().GetStringVal ("AutoCmdLine");
}

// Alternative Editor

bool Registry::UserDifferPrefs::IsAlternativeEditor ()
{
	unsigned long value;
	bool result = _alternativeEdit.Key ().GetValueLong ("On", value);
	return result && (value == 1);
}

std::string Registry::UserDifferPrefs::GetAlternativeEditorPath ()
{
	return _alternativeEdit.Key ().GetStringVal ("Path");
}

std::string Registry::UserDifferPrefs::GetAlternativeEditorCmdLine ()
{
	return _alternativeEdit.Key ().GetStringVal ("CmdLine");
}

void Registry::UserDifferPrefs::ToggleAlternativeEditor (bool isOn)
{
	_alternativeEdit.Key ().SetValueLong ("On", isOn ? 1: 0);
}

void Registry::UserDifferPrefs::SetAlternativeEditor (std::string const & exePath, 
													  std::string const & cmdLine)
{
	_alternativeEdit.Key ().SetValueString ("Path", exePath);
	_alternativeEdit.Key ().SetValueString ("CmdLine", cmdLine);
}

void Registry::UserDifferPrefs::RememberFont (unsigned tabSize, Font::Maker const & fontMaker)
{
	_differFont.Key ().SetValueString ("Name", fontMaker.GetFaceName ());
	_differFont.Key ().SetValueLong ("Weight", fontMaker.GetWeight ());
	_differFont.Key ().SetValueLong ("Size", -fontMaker.GetHeight ());
	if (fontMaker.IsItalic ())
		_differFont.Key ().SetValueLong ("Italic", 1);
	else
		_differFont.Key ().SetValueLong ("Italic", 0);
	_differFont.Key ().SetValueLong ("TabSize", tabSize);
}

bool Registry::UserDifferPrefs::GetFont (unsigned & tabSize, Font::Maker & fontMaker)
{
	unsigned long binVal;
	if (_differFont.Key ().GetValueLong ("TabSize", binVal))
		tabSize = binVal;
	std::string faceName = _differFont.Key ().GetStringVal ("Name");
	if (!faceName.empty ())
	{
		fontMaker.SetFaceName (faceName);
		_differFont.Key ().GetValueLong ("Weight", binVal);
		fontMaker.SetWeight (binVal);
		_differFont.Key ().GetValueLong ("Size", binVal);
		fontMaker.SetHeight (-(static_cast<int>(binVal)));
		_differFont.Key ().GetValueLong ("Italic", binVal);
		fontMaker.SetItalic (binVal == 1);
		return true;
	}
	return false;
}

void Registry::UserDifferPrefs::SetUseTabs (bool yes)
{
	_differFont.Key ().SetValueLong ("UseTabs", yes);
}

bool Registry::UserDifferPrefs::GetUseTabs () const
{
	unsigned long val = 1;
	if (_differFont.Key().IsValuePresent("UseTabs"))
	{
		_differFont.Key ().GetValueLong ("UseTabs", val);
	}
	return val != 0;
}

void Registry::UserDifferPrefs::SetTabSize (unsigned tabSizeChar)
{
	_differFont.Key ().SetValueLong ("TabSize", tabSizeChar);
}

unsigned Registry::UserDifferPrefs::GetTabSize () const
{
	unsigned long val = 0;
	_differFont.Key ().GetValueLong ("TabSize", val);
	return val;
}

bool Registry::UserDifferPrefs::GetSplitRatio (int & ratio)
{
	unsigned long value;
	bool result = _prefs.Key ().GetValueLong ("Split Ratio", value);
	ratio = value;
	return result;
}

void Registry::UserDifferPrefs::SetSplitRatio (int ratio)
{
	_prefs.Key ().SetValueLong ("Split Ratio", ratio);
}

bool Registry::UserDifferPrefs::IsLineBreakingOn ()
{
	unsigned long value;
	bool result = _prefs.Key ().GetValueLong ("Line Breaking", value);
	return result && (value == 1);
}

void Registry::UserDifferPrefs::SaveLineBreaking (bool on)
{
	_prefs.Key ().SetValueLong ("Line Breaking", on? 1: 0);
}

bool Registry::UserDifferPrefs::IsNoCheckoutPrompt ()
{
	unsigned long value;
	bool result = _prefs.Key ().GetValueLong ("No Checkout Prompt", value);
	return result && (value == 1);
}

void Registry::UserDifferPrefs::SetNoCheckoutPrompt (bool val)
{
	_prefs.Key ().SetValueLong ("No Checkout Prompt", val? 1: 0);
}

void Registry::UserDifferPrefs::SaveFindDlgPosition (int xLeft, int yTop, int numberPane)
{
	if (numberPane == 1)
	{
		_differFind.Key ().SetValueLong ("OnePaneX", xLeft); 
		_differFind.Key ().SetValueLong ("OnePaneY", yTop);
	}
	else
	{		
		_differFind.Key ().SetValueLong ("TwoPaneX", xLeft);
		_differFind.Key ().SetValueLong ("TwoPaneY", yTop);
	}
}

void Registry::UserDifferPrefs::GetFindDlgPosition (int & xLeft1, int & yTop1, int & xLeft2, int & yTop2)
{
	unsigned long value;
	xLeft1 = _differFind.Key ().GetValueLong ("OnePaneX", value) ? value : 0;
	yTop1 = _differFind.Key ().GetValueLong ("OnePaneY", value) ? value : 0;
	xLeft2 = _differFind.Key ().GetValueLong ("TwoPaneX", value) ? value : 0;
	yTop2 = _differFind.Key ().GetValueLong ("TwoPaneY", value) ? value : 0;
}

void Registry::UserDifferPrefs::SaveFindList (std::list <std::string> & list)
{
	std::string keyName("Find");
	SaveDialogFindList (keyName, list);			
}

void Registry::UserDifferPrefs::GetFindList (std::list <std::string> & list)
{
	std::string keyName("Find");
	LoadDialogFindList (keyName, list);			
}

void Registry::UserDifferPrefs::SaveReplaceList (std::list <std::string> & list)
{
	std::string keyName("Replace");
	SaveDialogFindList (keyName, list);
}

void Registry::UserDifferPrefs::GetReplaceList (std::list <std::string> & list)
{
	std::string keyName("Replace");
	LoadDialogFindList (keyName, list);
}

void Registry::UserDifferPrefs::SaveAssociationList (std::list <std::string> & listFind, std::list <std::string> & listReplace)
{
	std::string keyName("AssocFind");
	SaveDialogFindList (keyName, listFind);
	keyName.assign ("AssocReplace");
	SaveDialogFindList (keyName, listReplace);
}

void Registry::UserDifferPrefs::GetAssociationList (std::list <std::string> & listFind, std::list <std::string> & listReplace)
{
	std::string keyName("AssocFind");
	LoadDialogFindList (keyName, listFind);
	int count = listFind.size ();
	bool loadEmpty = true;
	keyName.assign ("AssocReplace");
	LoadDialogFindList (keyName, listReplace, loadEmpty, count);
}

void Registry::UserDifferPrefs::SaveFindPref (bool wholeWord, bool matchCase)
{
	_differFind.Key ().SetValueLong ("Whole Word", wholeWord? 1: 0);
	_differFind.Key ().SetValueLong ("Match Case", matchCase? 1: 0);
}

void Registry::UserDifferPrefs::GetFindPref (bool & wholeWord, bool & matchCase)
{
	unsigned long value;
	wholeWord = _differFind.Key ().GetValueLong ("Whole Word", value) && (value == 1);
	matchCase = _differFind.Key ().GetValueLong ("Match Case", value) && (value == 1);
}

std::string Registry::UserDifferPrefs::GetFindWord ()
{
	return _differFindlist.Key ().GetStringVal ("Find0");
}

void Registry::UserDifferPrefs::SaveDialogFindList (std::string & key, std::list <std::string> & list)
{
	std::list <std::string>::iterator it = list.begin ();
	for (int idx = 0; it != list.end () && idx < 10; it++, idx++)
	{
		std::string indexedKey = key + ToString <int> (idx);
		_differFindlist.Key ().SetValueString (indexedKey.c_str (), (*it).c_str ());
	}
}

void Registry::UserDifferPrefs::LoadDialogFindList (std::string & key,
													std::list <std::string> & list,
													bool loadEmpty,
													int count)
{
	for (int idx = 0; idx < count; idx++)
	{
		std::string indexedKey = key + ToString <int> (idx);
		std::string word = _differFindlist.Key ().GetStringVal (indexedKey.c_str ());
		if (!loadEmpty  && word.empty ())
			break;
		list.push_back (word);				
	}
}


void Registry::SetUserName (std::string const & name)
{
	Registry::UserInfo keyCoopUser;
	keyCoopUser.SetUserName (name.c_str ());
}

std::string Registry::GetUserName ()
{
	Registry::UserInfo keyCoopUser;
	return keyCoopUser.GetName ();
}

bool Registry::GetAutoInvitationOptions (std::string & path)
{
	Registry::UserPreferences prefs;
	std::string autoInvite = prefs.GetOption ("Auto Invitation");
	path = prefs.GetFilePath ("Auto Invitation Project Path");
	if (path.empty ())
		path = prefs.GetFilePath ("Recent Project Path");
	return (autoInvite == "yes");
}

void Registry::SetAutoInvitationOptions (bool isOn, std::string const & path)
{
	Registry::UserPreferences prefs;
	if (isOn)
	{
		prefs.SaveOption ("Auto Invitation", "yes");
		prefs.SaveFilePath ("Auto Invitation Project Path", path);
	}
	else
	{
		prefs.SaveOption ("Auto Invitation", "no");
	}
}

bool Registry::IsQuietConflictOption ()
{
	Registry::UserPreferences prefs;
	std::string quiet = prefs.GetOption ("Quiet Conflicts");
	return (quiet == "yes");
}

void Registry::SetQuietConflictOption (bool quiet)
{
	Registry::UserPreferences prefs;
	prefs.SaveOption ("Quiet Conflicts", quiet? "yes": "no");
}

std::string Registry::UserWikiPrefs::GetPropertyValue (std::string const & prop)
{
	return _prefs.Key ().GetStringVal (prop);
}
void Registry::UserWikiPrefs::SetPropertyValue (std::string const & propName, std::string const & propValue)
{
	_prefs.Key ().SetValueString (propName, propValue);
}
