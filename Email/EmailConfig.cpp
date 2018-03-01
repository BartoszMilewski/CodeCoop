// ----------------------------------
// (c) Reliable Software, 2002 - 2008
// ----------------------------------

#include "precompiled.h"
#include "EmailConfig.h"
#include "Registry.h"
#include "OutputSink.h"
#include "Validators.h"
#include "EmailConfigData.h"
#include "EmailRegistry.h"
#include "EmailAccount.h"
#include "ScriptProcessorConfig.h"
#include "Global.h"

#include <Win/Message.h>

const char Email::RegConfig::MAIL_ACCOUNT_KEY [] = "Mail Account";
const char Email::RegConfig::MAIL_ACCOUNT_TMP_KEY [] = "Mail Account Tmp";

const char Email::RegConfig::EMAIL_IN_NAME [] = "Email In";
const char Email::RegConfig::EMAIL_OUT_NAME [] = "Email Out";
const char Email::RegConfig::EMAIL_SIZE_NAME [] = "Max Email Size";
const char Email::RegConfig::AUTO_RECEIVE_NAME [] = "Auto Receive Period";
const char Email::RegConfig::EMAIL_TEST_NAME [] = "Email Config Passed";
const char Email::RegConfig::PREPRO_CMD_NAME [] = "Prepro Command";
const char Email::RegConfig::PREPRO_RESULT_NAME [] = "Prepro Result";
const char Email::RegConfig::POSTPRO_EXT_NAME [] = "Postpro Extension";
const char Email::RegConfig::POSTPRO_CMD_NAME [] = "Postpro Command";
const char Email::RegConfig::PREPRO_PROJECT_NAME [] = "Prepro Needs Project Name";
const char Email::RegConfig::PREPRO_UNPROCESSED_NAME [] = "Prepro Can Send Unprocessed";

unsigned int Min2Milisec (unsigned int minutes)
{
	return minutes * 60000;
}

unsigned int Milisec2Min (unsigned int milisec)
{
	return milisec / 60000;
}

bool Email::RegConfig::IsValuePresent (std::string const & valueName) const
{
	Registry::DispatcherEmail email (_keyName);
	return email.Key ().IsValuePresent (valueName);
}

void Email::RegConfig::BeginEdit ()
{
	Assert (_keyName == MAIL_ACCOUNT_KEY);
	Registry::DispatcherEmail currentEmail (_keyName);
	Registry::DispatcherEmail editEmail (MAIL_ACCOUNT_TMP_KEY);
	RegKey::CopyTree (currentEmail.Key (), editEmail.Key ());
	_keyName.assign (MAIL_ACCOUNT_TMP_KEY);
}

void Email::RegConfig::CommitEdit ()
{
	if (_keyName == MAIL_ACCOUNT_KEY)
		return; // there was no transaction

	Registry::DispatcherEmail newEmailConfig (_keyName);
	Registry::DispatcherEmail oldEmailConfig (MAIL_ACCOUNT_KEY);
	RegKey::CopyTree (newEmailConfig.Key (), oldEmailConfig.Key ());
	_keyName.assign (MAIL_ACCOUNT_KEY);
}

void Email::RegConfig::AbortEdit ()
{
	// Leave MAIL_ACCOUNT_TMP_KEY untouched
	_keyName.assign (MAIL_ACCOUNT_KEY);
}

void Email::RegConfig::SetDefaults ()
{
	SetMaxEmailSize (ChunkSizeValidator::GetDefaultChunkSize ());
	SetAutoReceive (DefaultAutoReceivePeriod);
	SetEmailStatus (Email::NotTested);
}

bool Email::RegConfig::IsSmtpRegKey () const
{
	Registry::DispatcherEmailTest test (_keyName);
	return test.ExistsSubKey ("SMTP");
}

bool Email::RegConfig::IsPop3RegKey () const
{
	Registry::DispatcherEmailTest test (_keyName);
	return test.ExistsSubKey ("POP3");
}

RegKey::AutoHandle Email::RegConfig::GetSmtpRegKey () const
{
	return Registry::DispatcherEmailAutoHandle (_keyName, "SMTP");
}

RegKey::AutoHandle Email::RegConfig::GetPop3RegKey () const
{
	return Registry::DispatcherEmailAutoHandle (_keyName, "POP3");
}

void Email::RegConfig::SetMaxEmailSize (unsigned long newSize) 
{
	Registry::DispatcherEmail email (_keyName);
	email.Key ().SetValueLong (EMAIL_SIZE_NAME, newSize);
}

unsigned long Email::RegConfig::GetMaxEmailSize () const 
{ 
	Registry::DispatcherEmail email (_keyName);
	unsigned long maxEmailSize = 0;
	if (email.Key ().GetValueLong (EMAIL_SIZE_NAME, maxEmailSize))
	{
		// Registry value present
		ChunkSizeValidator validator (maxEmailSize);
		if (validator.IsInValidRange ())
			return maxEmailSize;	// Registry value in valid range
	}

	// No registry value or value out of valid range -- return default email size
	maxEmailSize = ChunkSizeValidator::GetDefaultChunkSize ();
	return maxEmailSize; 
}

// auto receive
void Email::RegConfig::SetAutoReceive (unsigned int newPeriod) 
{ 
	Registry::DispatcherEmail email (_keyName);
	email.Key ().SetValueLong (AUTO_RECEIVE_NAME, newPeriod);
}

void Email::RegConfig::StopAutoReceive () 
{ 
	SetAutoReceive (0);
}

bool Email::RegConfig::IsAutoReceive () const 
{ 
	Registry::DispatcherEmail email (_keyName);
	unsigned long period = 0;
	if (email.Key ().GetValueLong (AUTO_RECEIVE_NAME, period))
		return period != 0;

	// No registry value - we will use default auto receive period
	return true;
}

unsigned int Email::RegConfig::GetAutoReceivePeriod () const 
{ 
	Assert (IsAutoReceive ());
	Registry::DispatcherEmail email (_keyName);
	unsigned long period = 0;
	if (email.Key ().GetValueLong (AUTO_RECEIVE_NAME, period))
	{
		// Registry value present
		if (MinAutoReceivePeriod <= period && period <= MaxAutoReceivePeriod)
			return period;
	}

	// No registry value or value out of valid range -- return default email size
	period = DefaultAutoReceivePeriod;
	return period;
}

// for UI usage only
unsigned int Email::RegConfig::GetAutoReceivePeriodInMin () const 
{ 
	return IsAutoReceive () ? 
		Milisec2Min (GetAutoReceivePeriod ()) : DefaultAutoReceivePeriodInMin;
}
void Email::RegConfig::SetAutoReceivePeriodInMin (unsigned int periodInMinutes)
{
	SetAutoReceive (Min2Milisec (periodInMinutes));
}

void Email::RegConfig::SetEmailStatus (Email::Status status) 
{ 
	Registry::DispatcherEmail email (_keyName);
	email.Key ().SetValueLong (EMAIL_TEST_NAME, status);
}

Email::Status Email::RegConfig::GetEmailStatus () const 
{ 
	Registry::DispatcherEmail email (_keyName);
	unsigned long val = 0;
	if (email.Key ().GetValueLong (EMAIL_TEST_NAME, val))
	{
		if (Email::NotTested <= val && val <= Email::Failed)
			return static_cast<Email::Status>(val);
	}

	// No registry value or value out of valid range
	val = Email::NotTested;
	return static_cast<Email::Status>(val);
}

bool Email::RegConfig::IsUsingSmtp () const
{
	Registry::DispatcherEmail email (_keyName);
	if (email.Key ().IsValuePresent (EMAIL_OUT_NAME))
	{
		std::string value = email.Key ().GetStringVal (EMAIL_OUT_NAME);
		return value == "SMTP";
	}
	return false;
}

void Email::RegConfig::SetIsUsingSmtp (bool isUsing) const
{
	Registry::DispatcherEmail email (_keyName);
	if (isUsing)
	{
		email.Key ().SetValueString (EMAIL_OUT_NAME, "SMTP");
		return;
	}

	if (!IsSimpleMapiForced ())
	{
		Registry::DefaultEmailClient defaultEmailProgram;
		if (defaultEmailProgram.IsMapiEmailClient ())
		{
			email.Key ().SetValueString (EMAIL_OUT_NAME, "MAPI");
			return;
		}
	}

	email.Key ().SetValueString (EMAIL_OUT_NAME, "SimpleMAPI");
}

bool Email::RegConfig::IsUsingPop3 () const
{
	Registry::DispatcherEmail email (_keyName);
	if (email.Key ().IsValuePresent (EMAIL_IN_NAME))
	{
		std::string value = email.Key ().GetStringVal (EMAIL_IN_NAME);
		return value == "POP3";
	}
	return false;
}

void Email::RegConfig::SetIsUsingPop3 (bool isUsing) const
{
	Registry::DispatcherEmail email (_keyName);
	if (isUsing)
	{
		email.Key ().SetValueString (EMAIL_IN_NAME, "POP3");
		return;
	}

	if (!IsSimpleMapiForced ())
	{
		Registry::DefaultEmailClient defaultEmailProgram;
		if (defaultEmailProgram.IsMapiEmailClient ())
		{
			email.Key ().SetValueString (EMAIL_IN_NAME, "MAPI");
			return;
		}
	}

	email.Key ().SetValueString (EMAIL_IN_NAME, "SimpleMAPI");
}

bool Email::RegConfig::IsSimpleMapiForced () const
{
	Registry::DispatcherEmail email (_keyName);
	unsigned long val = 0xffffffff;
	if (email.Key ().GetValueLong ("Mapi", val))
	{
		// val = 0 -- force simple Mapi; 1 -- full Mapi can be used
		return val == 0;
	}
	// Value not present -- nothing forced
	return false;
}

void Email::RegConfig::ForceSimpleMapi ()
{
	Registry::DispatcherEmail email (_keyName);
	// val = 0 -- force simple Mapi; 1 -- full Mapi can be used
	email.Key ().SetValueLong ("Mapi", 0);
}

bool Email::RegConfig::IsScriptProcessorPresent () const
{
	Registry::DispatcherEmail email (_keyName);
	return email.Key ().IsValuePresent (PREPRO_CMD_NAME) &&
		   email.Key ().IsValuePresent (PREPRO_RESULT_NAME) &&
		   email.Key ().IsValuePresent (POSTPRO_EXT_NAME) &&
		   email.Key ().IsValuePresent (POSTPRO_CMD_NAME) &&
		   email.Key ().IsValuePresent (PREPRO_PROJECT_NAME) &&
		   email.Key ().IsValuePresent (PREPRO_UNPROCESSED_NAME);
}

void Email::RegConfig::ReadScriptProcessorConfig (ScriptProcessorConfig & cfg) const
{
	Registry::DispatcherEmail email (_keyName);
	cfg.SetPreproCommand (email.Key ().GetStringVal (PREPRO_CMD_NAME));
	cfg.SetPreproResult (email.Key ().GetStringVal (PREPRO_RESULT_NAME));
	cfg.SetPostproCommand (email.Key ().GetStringVal (POSTPRO_CMD_NAME));
	cfg.SetPostproExt (email.Key ().GetStringVal (POSTPRO_EXT_NAME));

	unsigned long val = 0;
	email.Key ().GetValueLong (PREPRO_PROJECT_NAME, val);
	cfg.SetPreproNeedsProjName (val == 1);
	val = 0;
	email.Key ().GetValueLong (PREPRO_UNPROCESSED_NAME, val);
	cfg.SetCanSendUnprocessed (val == 1);
}

void Email::RegConfig::SaveScriptProcessorConfig (ScriptProcessorConfig const & cfg)
{
	Registry::DispatcherEmail email (_keyName);
	email.Key ().SetValueString (PREPRO_CMD_NAME, cfg.GetPreproCommand ());
	email.Key ().SetValueString (PREPRO_RESULT_NAME, cfg.GetPreproResult ());
	email.Key ().SetValueString (POSTPRO_CMD_NAME, cfg.GetPostproCommand ());
	email.Key ().SetValueString (POSTPRO_EXT_NAME, cfg.GetPostproExt ());

	email.Key ().SetValueLong (PREPRO_PROJECT_NAME, cfg.PreproNeedsProjName () ? 1 : 0);
	email.Key ().SetValueLong (PREPRO_UNPROCESSED_NAME, cfg.CanSendUnprocessed () ? 1 : 0);
}

void Email::RegConfig::Dump (std::ostream & out) const
{
	Registry::DispatcherEmail email (_keyName);
	if (email.Key ().IsValuePresent (EMAIL_IN_NAME))
		out << "*Email In: " << email.Key ().GetStringVal (EMAIL_IN_NAME) << std::endl;

	if (email.Key ().IsValuePresent (EMAIL_OUT_NAME))
		out << "*Email Out: " << email.Key ().GetStringVal (EMAIL_OUT_NAME) << std::endl;

	out << "*Max size of e-mail message: ";
	if (email.Key ().IsValuePresent (EMAIL_SIZE_NAME))
	{
		unsigned long val = 0;
		email.Key ().GetValueLong (EMAIL_SIZE_NAME, val);
		out << val << "kB" << std::endl;
	}
	else
		out << "undefined" << std::endl;

	out << "*Auto Receive Period: ";
	if (email.Key ().IsValuePresent (AUTO_RECEIVE_NAME))
	{
		unsigned long val = 0;
		email.Key ().GetValueLong (AUTO_RECEIVE_NAME, val);
		out << Milisec2Min (val) << " minutes" << std::endl;
	}
	else
		out << "undefined" << std::endl;

	if (IsUsingPop3 ())
	{
		Pop3Account pop3 (*this, std::string ());
		pop3.Dump (out);
	}
	if (IsUsingSmtp ())
	{
		SmtpAccount smtp (*this);
		smtp.Dump (out);
	}

	out << "*Script Processor Settings:" << std::endl;
	out << "**Preprocessor command: " << email.Key ().GetStringVal (PREPRO_CMD_NAME) << std::endl;
	out << "**Preprocessor result: " << email.Key ().GetStringVal (PREPRO_RESULT_NAME) << std::endl;
	out << "**Postprocessor command: " << email.Key ().GetStringVal (POSTPRO_CMD_NAME) << std::endl;
	out << "**Postprocessor extension: " << email.Key ().GetStringVal (POSTPRO_EXT_NAME) << std::endl;
	unsigned long val = 0;
	email.Key ().GetValueLong (PREPRO_PROJECT_NAME, val);
	out << "**Preprocessor needs project name: " << (val == 1 ? "YES" : "NO") << std::endl;

	val = 0;
	email.Key ().GetValueLong (PREPRO_UNPROCESSED_NAME, val);
	out << "**Preprocessor can send unprocessed: " << (val == 1 ? "YES" : "NO") << std::endl;
}
