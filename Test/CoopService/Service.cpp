//-----------------------------------
// (c) Reliable Software 2007
//-----------------------------------

#include "precompiled.h"
#include "Service.h"

#include <File/File.h>
#include <File/FileIo.h>
#include <Ex/WinEx.h>

#include <TimeStamp.h>

void Service::Installer::Install ()
{
	if (!File::Exists (_exePath))
		throw Win::Exception ("Cannot find service executable.", _exePath.c_str ());

	Service::AutoHandle serviceCtrlManager (
		::OpenSCManager (0,	// On this computer
						 SERVICES_ACTIVE_DATABASE,
						 SC_MANAGER_ALL_ACCESS));

	if (serviceCtrlManager.IsNull ())
		throw Win::Exception ("Cannot open service control manager database.");

	Service::AutoHandle service (
		::CreateService (serviceCtrlManager.ToNative (),
						 _serviceName.c_str (),		// Service name in service control manager database
						 _displayName.c_str (),		// Display service name
						 SC_MANAGER_ALL_ACCESS,		// Desired access rights
						 SERVICE_WIN32_OWN_PROCESS,	// Service that runs in its own process.
						 SERVICE_DEMAND_START,		// A service started by the service control manager when a process calls the StartService function.						  SERVICE_ERROR_NORMAL,		// The startup program logs the error and puts up a message box pop-up but continues the startup operation.
						 SERVICE_ERROR_NORMAL,		// The startup program logs the error and puts up a message box pop-up but continues the startup operation.
						 _exePath.c_str (),
						 0,							// No load ordering group
						 0,							// No tag identifier
						 0,							// No dependencies
						 0,							// LocalSystem account
						 0));						// No password

	if (service.IsNull ())
		throw Win::Exception ("Cannot create service.", _displayName.c_str ());
}

void Service::Installer::Deinstall ()
{
	Service::AutoHandle serviceCtrlManager (
		::OpenSCManager (0,	// On this computer
						 SERVICES_ACTIVE_DATABASE,
						 SC_MANAGER_ALL_ACCESS));

	if (serviceCtrlManager.IsNull ())
		throw Win::Exception ("Cannot open service control manager database.");

	Service::AutoHandle service (
		::OpenService (serviceCtrlManager.ToNative (),
					   _serviceName.c_str (),	// Service name in service control manager database
					   SC_MANAGER_ALL_ACCESS));	// Desired access rights

	if (service.IsNull ())
		throw Win::Exception ("Cannot open service.", _serviceName.c_str ());

	if (::DeleteService (service.ToNative ()) == 0)
		throw Win::Exception ("Cannot remove service from the database.", _displayName.c_str ());
}

// Service log has format of a wiki table

// Log entry := "| " + Date + " | " + Pre-formated text " |"

// Example:
// | 06.06.07 | 
//  one line log entry |
// | 07.06.07 | 
//  line1 of log entry 
//  line2 of log entry 
//  line3 of log entry |
// | 08.06.07 | 
//  one line log entry |

void Service::Base::Log::Write (std::string const & msg)
{
	if (_path.empty ())
		return;

	OutStream log (_path, std::ios_base::out | std::ios_base::app);
	StrTime now (CurrentTime ());
	log << "| " << now.GetString () << " | \n ";

	// Write log entry as wiki pre-formated text
	std::string::size_type curr = 0;
	std::string::size_type end = msg.find ('\n');
	if (end == std::string::npos)
	{
		// One line log entry
		log << msg;
	}
	else
	{
		// Multiple line log entry
		do 
		{
			log << msg.substr (curr, end - curr) << "\n ";
			curr = end + 1;
			end = msg.find ('\n', curr);
		} while (end != std::string::npos);

		log << msg.substr (curr, msg.length () - curr + 1);
	}

	log << '|' << std::endl;
}

Service::Base::Context Service::Base::_context;

Service::Base::Base (std::string const & name, std::string const & displayName)
	: _name (name),
	  _displayName (displayName)
{
	_context._serviceName = &_name;
	_context._thisService = this;
	_context._statusHandle = 0;
}

bool Service::Base::Initialize ()
{
	SERVICE_TABLE_ENTRY entryTable [] =
	{
		{ &_displayName [0],	Service::Base::ServiceMain },
		{ 0,					0						   }
	};

	return ::StartServiceCtrlDispatcher (entryTable) == 0;
}

void WINAPI Service::Base::ServiceMain (DWORD argc, LPTSTR * argv)
{
	std::vector<std::string> args;
	for (unsigned i = 0; i < argc; ++i)
		args.push_back (argv [i]);

	_context._statusHandle = ::RegisterServiceCtrlHandlerEx (_context._serviceName->c_str (),
															 Service::Base::ServiceControlHandlerEx,
															 &_context);
	if (_context._statusHandle == 0)
		return;

	Service::Status	status;
	status.SetRunning ();
	if (::SetServiceStatus (_context._statusHandle, &status) == 0)
		return;

	_context._thisService->Run (args);
}

DWORD WINAPI Service::Base::ServiceControlHandlerEx (DWORD dwControl,
													 DWORD dwEventType,
													 LPVOID lpEventData,
													 LPVOID lpContext)
{
	if (lpContext == 0)
		return ERROR_CALL_NOT_IMPLEMENTED;

	Service::Base::Context * context = reinterpret_cast<Service::Base::Context *>(lpContext);
	if (context->_thisService == 0)
		return ERROR_CALL_NOT_IMPLEMENTED;

	switch (dwControl)
	{
	case SERVICE_CONTROL_STOP:
		{
			context->_thisService->OnStop ();
			Service::Status	status;
			status.SetStopped ();
			::SetServiceStatus (context->_statusHandle, &status);
		}
		break;

	case SERVICE_CONTROL_PAUSE:
		{
			context->_thisService->OnPause ();
			Service::Status	status;
			status.SetPaused ();
			::SetServiceStatus (context->_statusHandle, &status);
		}
		break;

	case SERVICE_CONTROL_CONTINUE:
		{
			Service::Status	status;
			context->_thisService->OnContinue (status);
			::SetServiceStatus (context->_statusHandle, &status);
		}
		break;

	case SERVICE_CONTROL_INTERROGATE:
		{
			Service::Status	status;
			context->_thisService->OnInterrogate (status);
			::SetServiceStatus (context->_statusHandle, &status);
		}
		break;

	default:
		if (CustomCommandMin <= dwControl && dwControl <= CustomCommnadMax)
		{
			context->_thisService->OnCustomCommand (dwControl);
		}
		else
		{
			return ERROR_CALL_NOT_IMPLEMENTED;
		}
	}

	return NO_ERROR;
}
