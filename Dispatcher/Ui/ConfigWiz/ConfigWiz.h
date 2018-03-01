#if !defined (CONFIGWIZ_H)
#define CONFIGWIZ_H
// -------------------------------
// (c) Reliable Software 1999-2006
// -------------------------------
#include <Win/Win.h>

class ConfigDlgData;

class ConfigWizard
{
public:
	ConfigWizard (ConfigDlgData & config)
		: _config (config)
	{}
	bool Execute ();
	
private:
	ConfigDlgData & _config;
};

class EmailConfigWizard
{
public:
	EmailConfigWizard (ConfigDlgData & config)
		: _config (config)
	{}
	bool Execute ();
	
private:
	ConfigDlgData & _config;
};

#endif
