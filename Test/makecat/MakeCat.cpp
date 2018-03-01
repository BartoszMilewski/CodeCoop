//-------------------------------------
// (c) Reliable Software 2000 - 03
//-------------------------------------

#include "precompiled.h"
#include "Catalog.h"
#include "Address.h"
#include "ScriptProcessorConfig.h"
#include "Transport.h"

#include <Ex/WinEx.h>
#include <Dbg/Assert.h>
#include <Ex/Error.h>
#include <iostream>

void ReadTransportList (std::string prefix, TransportArray & transports);
void ReadScriptProcessorOptions (ScriptProcessorConfig & processor);
void ReadAssociations (Catalog & cat, char const * stop1, char const * stop2);
void ReadProjects (Catalog & cat, char const * stop1, char const * stop2);
void ReadUsers (Catalog & cat, char const * stop1, char const * stop2, char const * stop3);
void ReadSatUsers (Catalog & cat);
void ReadRemoteHubs (Catalog & cat);
std::string ReadToEol ();

bool write = false;

int main (int count, char * args [])
{
	std::string sw ("-c");
	if (count > 1 && sw == args [1])
		write = true;

	try
	{
		Catalog cat (true); // create

		std::string str1, str2, str3;
		long num;
		std::cin >> str1 >> str2;
		Assert (str1 == "Trial");
		Assert (str2 == "start:");
		std::cin >> num;
		if (write)
			cat.SetTrialStart (num);

		std::cin >> str1;
		Assert (str1 == "Topology:");

		long topology;
		std::cin >> std::hex >> topology;

		std::cin >> str1 >> str2;
		Assert (str1 == "Hub");
		Assert (str2 == "Id:");
		std::string hubId = ReadToEol ();

		TransportArray hubTransports;
		ReadTransportList ("Hub", hubTransports);
		TransportArray myTransports;
		ReadTransportList ("My", myTransports);

		ScriptProcessorConfig scriptProcessor;
		ReadScriptProcessorOptions (scriptProcessor);

		if (write)
			cat.SetDispatcherSettings (topology,
									   myTransports,
									   hubTransports,
									   hubId,
									   scriptProcessor);

		std::cin >> str1;
		Assert (str1 == "Type");
		std::cin >> str1;
		Assert (str1 == "associations:");

		ReadAssociations (cat, "Project", "list");
		ReadProjects (cat, "User", "list");
		ReadUsers (cat, "Satellite", "user", "list");
		ReadSatUsers (cat);
		ReadRemoteHubs (cat);
	}
	catch (Win::Exception e)
	{
		std::cerr << "makecat: " << e.GetMessage () << std::endl;
		SysMsg msg (e.GetError ());
		if (msg)
			std::cerr << "System tells us: " << msg.Text ();
		std::string objectName (e.GetObjectName ());
		if (!objectName.empty ())
			std::cerr << "    " << objectName << std::endl;
	}
	catch (...)
	{
		Win::ClearError ();
		std::cerr << "makecat: Unknown problem\n";
	}
	return 0;
}

void ReadTransportList (std::string prefix, TransportArray & transports)
{
	std::string str1, str2;
	std::cin >> str1 >> str2;
	Assert (str1 == prefix);
	Assert (str2 == "transports:");
	ReadToEol ();

	std::cin >> str1;
	std::vector<Transport> tr;
	while (str1 != "Active")
	{
		std::cout << str1 << "\n";
		tr.push_back (Transport (str1));
		std::cin >> str1;
	}
	std::cin >> str2;
	Assert (str2 == "transport:");
	std::cin >> str1;
	Transport::Method active = Transport::Unknown;
	if (str1 == "network")
		active = Transport::Network;
	else if (str1 == "email")
		active = Transport::Email;
	else if (str1 == "Magi")
		active = Transport::Magi;
	else
		Assert (!"Unknown transport type");
	
	unsigned int i;
	for (i = 0; i < tr.size (); ++i)
	{
		if (tr [i].GetMethod () == active)
			break;
	}
	Assert (i < tr.size ());
	transports.Init (tr, active);
}

void ReadScriptProcessorOptions (ScriptProcessorConfig & processor)
{
	std::string str1, str2;

	std::cin >> str1;
	Assert (str1 == "Script");
	std::cin >> str1 >> str2;
	Assert (str1 == "processing");
	Assert (str2 == "options:");
	
	std::string preproCommand, postproCommand, preproResult, postproExt;
	std::cin >> str1;
	Assert (str1 == "Preprocessor:");
	preproCommand = ReadToEol ();

	std::cin >> str1;
	Assert (str1 == "Result");
	std::cin >> str1;
	Assert (str1 == "file:");
	preproResult = ReadToEol ();

	std::cin >> str1;
	Assert (str1 == "Postprocessor:");
	postproCommand = ReadToEol ();

	std::cin >> str1;
	Assert (str1 == "File");
	std::cin >> str1;
	Assert (str1 == "extension:");
	postproExt = ReadToEol ();

	processor.SetPreproCommand (preproCommand);
	processor.SetPostproCommand (postproCommand);
	processor.SetPreproResult (preproResult);
	processor.SetPostproExt (postproExt);

	std::cout << preproCommand << ", " << postproCommand << ", "
		<< preproResult << ", " << postproExt << std::endl;
}

void ReadAssociations (Catalog & cat, char const * stop1, char const * stop2)
{
	std::string str;
	std::cin >> str;
	while (str != stop1)
	{
		Assert (str == "Type:");
		long type;
		std::cin >> type;

		long num;
		std::cin >> str;
		Assert (str == "State:");
		std::cin >> num;

		std::cin >> str;
		Assert (str == "Extension:");
		std::string ext = ReadToEol ();
		if (write)
			cat.AddFileType (ext, FileType (type));
		std::cin >> str;
	}
	std::cin >> str;
	Assert (str == stop2);
	std::cout << std::endl;
}

void ReadProjects (Catalog & cat, char const * stop1, char const * stop2)
{
	std::string str, str2, name;
	std::cin >> str;
	
	while (str != stop1)
	{
		Assert (str == "Project");
		std::cin >> str;
		Assert (str == "id:");
		long projId;
		std::cin >> std::hex >> projId;

		std::cin >> str2;
		Assert (str2 == "State:");
		long num;
		std::cin >> num;

		std::cin >> str >> str2;
		Assert (str == "Project");
		Assert (str2 == "name:");
		name = ReadToEol ();

		std::cin >> str >> str2;
		Assert (str == "Source");
		Assert (str2 == "path:");
		str = ReadToEol ();

		std::cout << projId << " " << " " << name << " " << str << "\n";
		if (write)
			cat.AddProject (projId, name, str);

		std::cin >> str;
	}
	std::cin >> str;
	Assert (str == stop2);
	std::cout << std::endl;
}

void ReadUsers (Catalog & cat, char const * stop1, char const * stop2, char const * stop3)
{
	std::string str, str2;
	std::cin >> str;
	while (str != stop1)
	{
		long projId;
		Assert (str == "Project");
		std::cin >> str;
		Assert (str == "id:");
		std::cin >> std::hex >> projId;

		long num;
		std::cin >> str;
		Assert (str == "State:");
		std::cin >> num;

		std::cin >> str >> str2;
		Assert (str == "Project");
		Assert (str2 == "name:");
		std::string proj = ReadToEol ();

		std::cin >> str >> str2;
		Assert (str == "User");
		Assert (str2 == "HubId:");
		std::string hubId = ReadToEol ();

		std::cin >> str >> str2;
		Assert (str == "User");
		Assert (str2 == "Id:");
		// Notice: old enlistments may have empty user-id
		str = ReadToEol ();

		if (write)
			cat.AddProjectMember (Address (hubId, proj, str), projId);
		else
			std::cout << proj << " " << hubId << " " << str << " " << projId << "\n";
		
		std::cin >> str;
	}
	std::cin >> str;
	Assert (str == stop2);
	std::cin >> str;
	Assert (str == stop3);
	std::cout << std::endl;
}

void ReadSatUsers (Catalog & cat)
{
	std::string str, str2;
	std::cin >> str;

	while (str != "Remote")
	{
		long num;
		Assert (str == "State:");
		std::cin >> num;

		std::cin >> str >> str2;
		Assert (str == "Project");
		Assert (str2 == "name:");
		std::string proj = ReadToEol ();

		std::cin >> str >> str2;
		Assert (str == "User");
		Assert (str2 == "HubId:");
		std::string hubId = ReadToEol ();

		std::cin >> str >> str2;
		Assert (str == "User");
		Assert (str2 == "Id:");
		std::string userId = ReadToEol ();

		std::cin >> str;
		Assert (str == "Transport:");
		// Notice: transport may be empty
		str = ReadToEol ();
		std::cout << hubId << " " << proj << " " << userId << " " << str << "\n";
		if (write)
			cat.AddClusterRecipient (Address (hubId, 
											  proj,
											  userId), 
									 Transport (str));

		std::cin >> str;
	}
	std::cout << std::endl;
}

void ReadRemoteHubs (Catalog & cat)
{
	std::string str, str2;
	std::cin >> str >> str2;
	Assert (str == "hub");
	Assert (str2 == "list");

	while (std::cin >> str)
	{
		long num;
		Assert (str == "State:");
		std::cin >> num;

		std::cin >> str;
		Assert (str == "HubId:");
		std::string hubId = ReadToEol ();
		std::string route;
		std::cin >> route;
		std::string method = ReadToEol ();
		std::cout << hubId << "\n" << route << " " << method << "\n";
		if (write)
			cat.AddRemoteHub (hubId, Transport (route));
	}
}

std::string ReadToEol ()
{
	std::string str;
	char c;
	do
	{
		std::cin.get (c);
	} while (c == ' ');
	if (c != '\n')
	{
		std::cin.putback (c);
		std::getline (std::cin, str);
	}
	return str;
}
