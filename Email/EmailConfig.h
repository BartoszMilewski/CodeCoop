#if !defined (EMAILCONFIG_H)
#define EMAILCONFIG_H
// ----------------------------------
// (c) Reliable Software, 2002 - 2008
// ----------------------------------

class EmailConfigData;
class ScriptProcessorConfig;

namespace RegKey { class Handle; }
namespace RegKey { class AutoHandle; }

namespace Email
{
	// Don't change numbers: they are stored in the registry
	enum Status
	{
		NotTested = 0,
		Succeeded = 1,
		Failed = 2
	};

	const unsigned int MinAutoReceivePeriodInMin = 2; 
	const unsigned int MaxAutoReceivePeriodInMin = 9999; 
	const unsigned int DefaultAutoReceivePeriodInMin = 10;
	const unsigned int MinAutoReceivePeriod	     = MinAutoReceivePeriodInMin * 60 * 1000;
	const unsigned int MaxAutoReceivePeriod      = MaxAutoReceivePeriodInMin * 60 * 1000;
	const unsigned int DefaultAutoReceivePeriod  = DefaultAutoReceivePeriodInMin * 60 * 1000;

	// All over the code periods are measured in milliseconds
	// it is only UI that must use minutes
	// be careful when passing the periods to UI and when retrieving user input form UI

	class RegConfig
	{
		// data is read straight from registry, not cached
	private:
		static const char MAIL_ACCOUNT_KEY [];
		static const char MAIL_ACCOUNT_TMP_KEY [];

	public:
		// Email config value names
		static const char EMAIL_IN_NAME [];
		static const char EMAIL_OUT_NAME [];
		static const char EMAIL_SIZE_NAME [];
		static const char AUTO_RECEIVE_NAME [];
		static const char EMAIL_TEST_NAME [];
		static const char PREPRO_CMD_NAME [];
		static const char PREPRO_RESULT_NAME [];
		static const char POSTPRO_EXT_NAME [];
		static const char POSTPRO_CMD_NAME [];
		static const char PREPRO_PROJECT_NAME [];
		static const char PREPRO_UNPROCESSED_NAME [];

	public:
		RegConfig ()
			: _keyName (MAIL_ACCOUNT_KEY)
		{}

		bool IsValuePresent (std::string const & valueName) const;

		// Transaction
		void BeginEdit (); // copies old key to tmp key, changes _keyName
		void CommitEdit (); // copy over tmp key to old key, change _keyName back
		void AbortEdit (); // delete tmp key
		bool IsInTransaction () const { return _keyName == MAIL_ACCOUNT_TMP_KEY; }

		void SetDefaults ();

		// Return sub-keys of current key
		bool IsSmtpRegKey () const;
		bool IsPop3RegKey () const;
		RegKey::AutoHandle GetSmtpRegKey () const;
		RegKey::AutoHandle GetPop3RegKey () const;

		void SetMaxEmailSize (unsigned long newSize);
		unsigned long GetMaxEmailSize () const;

		// Auto receive
		void SetAutoReceive (unsigned int newPeriod);
		void StopAutoReceive ();
		bool IsAutoReceive () const;
		unsigned int GetAutoReceivePeriod () const;

		// For UI usage only
		void SetAutoReceivePeriodInMin (unsigned int periodInMinutes);
		unsigned int GetAutoReceivePeriodInMin () const;

		void SetEmailStatus (Email::Status status);
		Email::Status GetEmailStatus () const;

		bool IsUsingSmtp () const;
		void SetIsUsingSmtp (bool isUsing) const;
		bool IsUsingPop3 () const;
		void SetIsUsingPop3 (bool isUsing) const;
		bool IsSimpleMapiForced () const;
		void ForceSimpleMapi ();

		// Script processor
		bool IsScriptProcessorPresent () const;
		void ReadScriptProcessorConfig (ScriptProcessorConfig & cfg) const;
		void SaveScriptProcessorConfig (ScriptProcessorConfig const & cfg);

		void Dump (std::ostream & out) const;

	private:
		std::string	_keyName;
	};
}

#endif
