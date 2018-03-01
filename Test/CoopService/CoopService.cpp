//-----------------------------------
// (c) Reliable Software 2007
//-----------------------------------

#include "precompiled.h"
#include "Service.h"
#include "PathRegistry.h"

#include <File/File.h>
#include <File/Path.h>
#include <Ex/Error.h>
#include <StringOp.h>
#include <Ctrl/Output.h>

#include <iostream>

static char const * serviceName = "CoopService";
static LPSTR serviceDisplayName = "Code Co-op Test Service";

int InstallService ();
int RemoveService ();

class CoopTestService : public Service::Base
{
public:
	CoopTestService (std::string const & name, std::string const & displayName)
		: Service::Base (name, displayName),
		  _isRunnig (false)
	{}

	void Run (std::vector<std::string> const & args);
	void OnStop ();
	void OnPause ();
	void OnContinue (Service::Status & status);
	void OnInterrogate (Service::Status & status);
	void OnCustomCommand (unsigned command);

private:
	bool	_isRunnig;
};

void CoopTestService::Run (std::vector<std::string> const & args)
{
	std::string msg ("Code Co-op Test Service start\nArguments:\n");
	for (std::vector<std::string>::const_iterator iter = args.begin (); iter != args.end (); ++iter)
	{
		msg += *iter;
		msg += "\n";
	}
	_log.Write (msg);

	while (_isRunnig)
	{
		::Sleep (2000);
	}
}

void CoopTestService::OnStop ()
{
	_log.Write ("CoopTestService::OnStop");
	_isRunnig = false;
}

void CoopTestService::OnPause ()
{
	_log.Write ("CoopTestService::OnPause");
}

void CoopTestService::OnContinue (Service::Status & status)
{
	_log.Write ("CoopTestService::OnContinue");
}

void CoopTestService::OnInterrogate (Service::Status & status)
{
	_log.Write ("CoopTestService::OnInterrogate");
	if (_isRunnig)
		status.SetRunning ();
	else
		status.SetStopped ();
}

void CoopTestService::OnCustomCommand (unsigned command)
{
	_log.Write ("CoopTestService::OnCustomCommand");
}

static std::auto_ptr<CoopTestService> TheService;

int main (int argc, char * argv [])
{
	if (argc > 1)
	{
		std::string arg (argv [1]);
		if (IsNocaseEqual (arg, "-i"))
			return InstallService ();
		else if (IsNocaseEqual (arg, "-d"))
			return RemoveService ();

		std::cerr << "Usage: coopservice -i - for installation" << std::endl;
		std::cerr << "       coopservice -d - for de-installation" << std::endl;
		std::cerr << "       coopservice    - to start installed service" << std::endl;
	}
	else
	{
		TheService.reset (new CoopTestService (serviceName, serviceDisplayName));
		FilePath logPath (Registry::GetLogsFolder ());
		std::string fileName (serviceName);
		fileName += ".wiki";
		TheService->SetLogPath (logPath.GetFilePath (fileName));
		if (!TheService->Initialize ())
		{
			LastSysErr lastError;
			std::cerr << "Cannot start Code Co-op service." << std::endl;
			std::cerr << "System tells us: " << lastError.Text () << std::endl;
			TheService.reset (0);
		}
	}
	return 0;
}

int InstallService ()
{
	CurrentFolder currentFolder;
	char const * exePath = currentFolder.GetFilePath ("CoopService.exe");

	Service::Installer installer (serviceName, serviceDisplayName, exePath);
	try
	{
		installer.Install ();
		return 0;
	}
	catch (Win::Exception ex)
	{
		std::cerr << Out::Sink::FormatExceptionMsg (ex) << std::endl;
	}
	return 1;
}

int RemoveService ()
{
	CurrentFolder currentFolder;
	char const * exePath = currentFolder.GetFilePath ("CoopService.exe");

	Service::Installer installer (serviceName, serviceDisplayName, exePath);
	try
	{
		installer.Deinstall ();
		return 0;
	}
	catch (Win::Exception ex)
	{
		std::cerr << Out::Sink::FormatExceptionMsg (ex) << std::endl;
	}
	return 1;
}
