#if !defined (REGISTRY_H)
#define REGISTRY_H
//----------------------------------
// (c) Reliable Software 2000 - 2008
//----------------------------------

#include "RegKeys.h"

#include <Sys/RegKey.h>

class MemberDescription;
class FilePath;
class ScriptProcessorConfig;

namespace Win 
{ 
	class Placement;
}
namespace Font
{
	// Wiesiek Revisit: don't use Maker.
	// Pass Font::Hanlde and use Font::Logical to get description
	class Maker;
}

namespace Registry
{
	bool CanWriteToMachineRegKey ();

	// user data
	class RecentProjectList
	{
		static const unsigned int RecentProjectCount = 5;
	public:
		typedef std::vector<unsigned int>::const_iterator const_iterator;
	public:
		RecentProjectList ();
		void Add (unsigned int id);
		const_iterator begin () const { return _ids.begin (); }
		const_iterator end () const { return _ids.end (); }
	private:
		std::vector<unsigned int>	_ids;
	};
	std::string GetRecentProject ();
	void SetRecentProject (char const * projDir);

    void StoreUserDescription (MemberDescription const & user);
	void ReadUserDescription (MemberDescription & user);
	void SetUserName (std::string const & name);
	std::string GetUserName ();
	bool GetAutoInvitationOptions (std::string & path);
	void SetAutoInvitationOptions (bool isOn, std::string const & path);
	bool IsQuietConflictOption ();
	void SetQuietConflictOption (bool ignore);

	void SetNagging (bool flag);
	bool IsNagging ();

	// Beginner Mode
	void SetBeginnerMode (bool on);
	void BeginnerMessageOff (int msgId);
	bool IsBeginnerMessageOff (int msgId);
	bool IsBeginnerModeOn ();
	int CountDisabledBeginner (int begin, int end);

	// Initialization
	bool IsFirstRun ();
	bool IsRestoredConfiguration ();

	class UserPreferences
	{
	public:
		RegKey::Handle & GetRootKey () { return _pref.Key (); }
		RegKey::Handle const & GetRootKey () const { return _pref.Key (); }
		void GetListWinPlacement (Win::Placement & placement);
		void SaveListWinPlacement (Win::Placement & placement);
		std::string GetFilePath (std::string const & pathName);
		void SaveFilePath (std::string const & pathName, std::string const & path);
		std::string GetOption (std::string const & optionNameName);
		void SaveOption (std::string const & optionName, std::string const & option);
		std::string GetOptionDecrypt (std::string const & optionNameName);
		void SaveOptionEncrypt (std::string const & optionName, std::string const & option);
		bool CanAsk (unsigned int promptId);
		void TurnOffPrompt (unsigned int promptId);

		void GetWinPlacement (std::string const & keyName, Win::Placement & placement) const;
		void SaveWinPlacement (std::string const & keyName, Win::Placement & placement);
	private:
		void Validate (Win::Placement & placement) const;
		void SaveStringValue (std::string const & keyName,
							  std::string const & valueName,
							  std::string const & value);
		std::string GetStringValue (std::string const & keyName,
									std::string const & valueName);
		void SaveStringValueEncrypt (std::string const & keyName,
									 std::string const & valueName,
									 std::string const & value);
		std::string GetStringValueDecrypt (std::string const & keyName,
										   std::string const & valueName);

	private:
		Registry::Preferences	_pref;
	};

	class UserDispatcherPrefs
	{
	public:
		bool IsFirstEmail () const;
		void SetFirstEmail (bool value);
		static bool IsInitialized ();

		bool IsSimpleMapiForcedObsolete () const;
		bool IsUsePop3Obsolete ();
		bool IsUseSmtpObsolete ();

		bool IsEmailLogging ();
		bool GetAutoReceivePeriodObsolete (unsigned long & period);
		bool GetMaxEmailSizeObsolete (unsigned long & size);
		void SetStayOffSiteHub (bool asked);
		bool IsStayOffSiteHub ();
		long GetEmailStatusObsolete ();
		bool GetResendDelay (unsigned long & delay);
		bool GetRepeatInterval (unsigned long & repeat);
		void SetAutoResend (unsigned delay, unsigned repeat);

		void GetMainWinPlacement (Win::Placement & placement);
		void SaveMainWinPlacement (Win::Placement & placement);
		void GetColumnWidths (char const * viewName, std::vector<int> & widths);
		void SetColumnWidths (char const * viewName, std::vector<int> const & widths);
		unsigned int GetSortCol (char const * viewName);
		void SetSortCol (char const * viewName, unsigned int col);
		
		// Version updates
		bool IsConfirmUpdate ();
		bool GetUpdateTime (int & year, int & month, int & day);
		unsigned int GetUpdatePeriod ();
		unsigned int GetLastBulletin ();
		std::string GetLastDownloadedVersion ();
		void SetUpdateTime (int year, int month, int day);
		void SetUpdatePeriod (unsigned int checkPeriod);
		void SetIsConfirmUpdate (bool isConfirmUpdate);
		void SetLastBulletin (unsigned int lastBulletin);
		void SetLastDownloadedVersion (std::string const & lastVersion);

		void ReadScriptProcessorConfigObsolete (ScriptProcessorConfig & cfg);

	private:
		Registry::DispatcherPrefs	_prefs;
	};

	class UserDifferPrefs
	{
	public:
		bool IsAlternativeDiffer (bool & isOn);
		bool IsAlternativeDifferValid ();
		std::string GetAlternativeDiffer (bool & useXml);
		std::string GetDifferCmdLine (bool twoArg);
		void SetAlternativeDiffer ( bool isOn, 
									std::string const & exePath, 
									std::string const & cmdLine,
									std::string const & cmdLine2,
									bool useXml);
		void ToggleAlternativeDiffer (bool isOn);


		bool IsAlternativeMerger (bool & isOn);
		bool IsAlternativeMergerValid ();
		std::string GetAlternativeMerger (bool & useXml);
		std::string GetMergerCmdLine ();
		void SetAlternativeMerger ( bool isOn, 
									std::string const & exePath, 
									std::string const & cmdLine,
									bool useXml);
		std::string GetAlternativeAutoMerger (bool & useXml);
		std::string GetAutoMergerCmdLine ();
		void SetAlternativeAutoMerger (std::string const & exePath, 
									   std::string const & cmdLine,
									   bool useXml);
		void ToggleAlternativeMerger (bool isOn);

		bool IsAlternativeEditor ();
		std::string GetAlternativeEditorPath ();
		std::string GetAlternativeEditorCmdLine ();
		void ToggleAlternativeEditor (bool isOn);
		void SetAlternativeEditor (std::string const & exePath,
								   std::string const & cmdLine);

		void RememberFont (unsigned tabSize, Font::Maker const & fontMaker);
		bool GetFont (unsigned & tabSize, Font::Maker & fontMaker);
		void SetTabSize (unsigned tabSize);
		unsigned GetTabSize () const;
		bool GetUseTabs() const;
		void SetUseTabs(bool yes);

		void SetSplitRatio (int ratio);
		bool GetSplitRatio (int & ratio);
		bool IsLineBreakingOn ();
		void SaveLineBreaking (bool on);
		bool IsNoCheckoutPrompt ();
		void SetNoCheckoutPrompt (bool val);

		void SaveFindDlgPosition (int xLeft, int yTop, int numberPane);
		void GetFindDlgPosition (int & xLeft1, int & yTop1, int & xLeft2, int & yTop2);
		void SaveFindPref (bool wholeWord, bool matchCase);
		void GetFindPref (bool & wholeWord, bool & matchCase);

		void SaveFindList (std::list <std::string> & list);
		void GetFindList (std::list <std::string> & list);
		std::string  GetFindWord ();
		void SaveReplaceList (std::list <std::string> & list);
		void GetReplaceList (std::list <std::string> & list);
		void SaveAssociationList (std::list <std::string> & listFind, std::list <std::string> & listReplace);
       	void GetAssociationList (std::list <std::string> & listFind, std::list <std::string> & listReplace);

	private:
		void SaveDialogFindList (std::string & key, std::list <std::string> & list);
		void LoadDialogFindList (std::string & key, std::list <std::string> & list,
								 bool loadEmpty = false, int count = 10);

	private:
		Registry::DifferPrefs	  _prefs;
		Registry::MergerAlternative	_alternativeMerge;
		Registry::DifferAlternative	_alternativeDiff;
		Registry::EditorAlternative	_alternativeEdit;
		Registry::DifferFont	  _differFont;
		Registry::DifferFind      _differFind;
		Registry::DifferFindList  _differFindlist;
	};

	class UserWikiPrefs
	{
	public:
		std::string GetPropertyValue (std::string const & prop);
		void SetPropertyValue (std::string const & propName, std::string const & propValue);

	private:
		Registry::WikiPrefs	  _prefs;
	};
}

#endif
