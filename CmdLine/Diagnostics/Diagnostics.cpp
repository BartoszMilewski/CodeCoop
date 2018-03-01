//------------------------------------
//  (c) Reliable Software, 2000 - 2008
//------------------------------------

#include "precompiled.h"
#include "CmdArgs.h"
#include "CmdFlags.h"
#include "SccProxyEx.h"
#include "Catalog.h"
#include "CmdLineVersionLabel.h"
#include "Crypt.h"
#include "GlobalMessages.h"

#include <Win/Message.h>
#include <Ex/WinEx.h>
#include <Ex/Error.h>
#include <XML/Scanner.h>
#include <XML/XmlParser.h>
#include <XML/XmlTree.h>

#include <iostream>
#include <fstream>

class DiagSwitch : public StandardSwitch
{
public:
	DiagSwitch ()
	{
		SetDumpVersion ();
		SetDumpCatalog ();
		SetDumpMembership ();
		SetDumpHistory ();
		SetDumpAll ();
		SetNotifySink ();
		SetPickLicense ();
	}
};

void DumpMembership (char const * diagFile, char const * errFile, bool pickLicense);

int main (int count, char * args [])
{
	int retCode = 0;
	Win::Dow::Handle notifySink;
	try
	{
		DiagSwitch validSwitch;
		CmdArgs cmdArgs (count, args, validSwitch);
		if (cmdArgs.IsHelp () || cmdArgs.IsEmpty ())
		{
			std::cout << CMDLINE_VERSION_LABEL << ".\n\n";
			std::cout << "Generate diagnostic report from all projects on this computer.\n\n";
			std::cout << "diagnostics <options> <diagnostic file path> <error file path>\n";
			std::cout << "options:\n";
			std::cout << "   -d:version -- include Windows and co-op version\n";
			std::cout << "   -d:catalog -- include catalog of projects and routing information\n";
			std::cout << "   -d:membership -- include information about project membership\n";
			std::cout << "   -d:history   -- include information about project history\n";
			std::cout << "   -d:all   -- include all information about project\n";
			std::cout << "   -? -- display help\n";
		}
		else
		{
			char const ** paths = cmdArgs.GetFilePaths ();
			char const * diagFile = paths [0];
			char const * errFile = 0;
			if (cmdArgs.Size () > 1)
				errFile = paths [1];
			XML::Tree diagTree;
			XML::Node * root = diagTree.SetRoot ("Diagnostics");
			{
				// Open XML root node
				std::ofstream out (diagFile);
				diagTree.WriteHeader (out);
				root->WriteOpeningTag (out, 0);
			}
			// Create Co-op command line
			std::string cmd ("Help_SaveDiagnostics");
			if (cmdArgs.IsDumpMembership () || cmdArgs.IsDumpAll ())
				cmd += " membership:\"yes\"";
			if (cmdArgs.IsDumpHistory () || cmdArgs.IsDumpAll ())
				cmd += " history:\"yes\"";

			cmd += " target:\"";
			cmd += diagFile;
			cmd += "\" overwrite:\"no\"";
			// First command dumps version and catalog if requested
			std::string firstCmd (cmd);
			if (cmdArgs.IsDumpVersion () || cmdArgs.IsDumpAll ())
				firstCmd += " version:\"yes\"";
			if (cmdArgs.IsDumpCatalog () || cmdArgs.IsDumpAll ())
				firstCmd += " catalog:\"yes\"";

			Win::UserMessage msg (UM_PROGRESS_TICK);
			if (cmdArgs.IsNotifySink ())
			{
				Win::Dow::Handle handle (reinterpret_cast<HWND>(cmdArgs.GetNotifySink ()));
				notifySink.Reset (handle.ToNative ());
			}

			SccProxyEx sccProxy;
			Catalog catalog;
			ProjectSeq seq (catalog);
			while (!seq.AtEnd ())
			{
				if (cmdArgs.IsNotifySink ())
					notifySink.PostMsg (msg);

				if (sccProxy.CoopCmd (seq.GetProjectId (), firstCmd, false, true))// Skip GUI Co-op in the project
				{																  // Execute command without timeout
					// First command executed successfully
					seq.Advance ();
					break;
				}
				else
				{
					std::string info (seq.GetProjectName ());
					info += " (";
					info += ToString (seq.GetProjectId ());
					info += ")";
					std::cerr << "diagnostics: Cannot generate diagnostics for the project: " << info << std::endl;
					retCode = 1;
					// Try execute the first command in the next project
					seq.Advance ();
				}
			}
			// Subsequent commands append dump the output file and don't repeat catalog dump
			for (; !seq.AtEnd (); seq.Advance ())
			{
				if (cmdArgs.IsNotifySink ())
					notifySink.PostMsg (msg);

				if (!sccProxy.CoopCmd (seq.GetProjectId (), cmd, false, true))// Skip GUI Co-op in the project
				{															  // Execute command without timeout
					std::string info (seq.GetProjectName ());
					info += " (";
					info += ToString (seq.GetProjectId ());
					info += ")";
					std::cerr << "diagnostics: Cannot generate diagnostics for the project: " << info << std::endl;
					retCode = 1;
				}
			}

			{
				// Close XML root node
				std::ofstream out;
				out.open (diagFile, std::ios::app);
				root->WriteClosingTag (out, 0);
			}

			bool dumpMembership = cmdArgs.IsDumpMembership () && cmdArgs.IsDumpCatalog ()
				|| cmdArgs.IsDumpAll ();

			if (dumpMembership)
				DumpMembership (diagFile, errFile, cmdArgs.IsPickLicense ());
		}
	}
	catch (Win::Exception e)
	{
		std::cerr << "diagnostics: " << e.GetMessage () << std::endl;
		SysMsg msg (e.GetError ());
		if (msg)
			std::cerr << "System tells us: " << msg.Text ();
		std::string objectName (e.GetObjectName ());
		if (!objectName.empty ())
			std::cerr << "    " << objectName << std::endl;
		retCode = 1;
	}
	catch (...)
	{
		Win::ClearError ();
		std::cerr << "diagnostics: Unknown problem\n";
		retCode = 1;
	}
	if (!notifySink.IsNull ())
	{
		Win::UserMessage msg (UM_TOOL_END);
		notifySink.PostMsg (msg);
	}
	return retCode;
}

//-----------------
// Storage Trees
//-----------------

namespace Tree
{
	class Node;
	typedef auto_vector<Node>::const_iterator ConstChildIter;

	class Node
	{
	public:
		Node (std::string const & name)
			: _name (name)
		{}
		Node * AddChild (std::unique_ptr<Node> child);
		Node * AddChild (std::string const & name);
		std::string const & GetName () const { return _name; }
		Node * FindChild (std::string const & name);
		ConstChildIter BeginChild () const { return _children.begin (); }
		ConstChildIter EndChild  () const { return _children.end  ();  }
		void Sort ();

		virtual void PrintOut (int indent, std::ostream & out = std::cout) const;
		static void Indent (int indent, std::ostream & out = std::cout)
		{
			for (int i = 0; i < indent; ++i)
				out << " ";
		}
	private:
		std::string _name;
		auto_vector<Node> _children;
	};

	int IsLess (Node const * n1, Node const * n2)
	{
		return NocaseCompare (n1->GetName (), n2->GetName ()) < 0;
	}

	Node * Node::AddChild (std::unique_ptr<Node> child)
	{
		_children.push_back (std::move(child));
		return _children.back ();
	}

	Node * Node::AddChild (std::string const & name)
	{
		std::unique_ptr<Node> child (new Node (name));
		return AddChild (std::move(child));
	}

	Node * Node::FindChild (std::string const & name)
	{
		ConstChildIter it = _children.begin ();
		while (it != _children.end ())
		{
			if (IsNocaseEqual ((*it)->GetName (), name))
				return *it;
			++it;
		}
		return 0;
	}

	void Node::Sort ()
	{
		std::sort (_children.begin (), _children.end (), IsLess);
		std::for_each (_children.begin (), _children.end (), std::mem_fun (&Node::Sort));
	}

	void Node::PrintOut (int indent, std::ostream & out) const
	{
		Indent (indent, out);
		out << _name << std::endl;
		for (ConstChildIter it = BeginChild (); it != EndChild (); ++it)
		{
			(*it)->PrintOut (indent + 4, out);
		}
	}

	class ListNode: public Node
	{
	public:
		ListNode (std::string const & name) : Node (name) {}
		void PrintOut (int indent, std::ostream & out = std::cout) const
		{
			Indent (indent, out);
			out << GetName () << " (";
			for (ConstChildIter it = BeginChild (); it != EndChild (); ++it)
			{
				out << " " << (*it)->GetName ();
			}
			out << " )\n";
		}
	};
}

//--------------------
// Membership Database
//--------------------
namespace Membership
{
	class Enlistment
	{
	public:
		Enlistment (std::string const & projName, std::string const & projId)
			: _projName (projName), _projId (projId)
		{}
		std::string const & _projName;
		std::string const & _projId;
	};

	class ErrorLog
	{
	public:
		std::string GetLog () { return _out.str ();	}
		void BadLocalHubId (
			std::string const & badHub, 
			std::string const & goodHub, 
			Enlistment const & enlistment,
			std::string const & user, 
			std::string const & memberId)
		{
			_out << "Your email (hub ID) in project " 
				<< enlistment._projName << " (project ID " << enlistment._projId << ") "
				<< "is different than the one known to your dispatcher (" << goodHub << std::endl;
			_out << "    FIX: Visit this project with Code Co-op and accept the change.\n\n";
		}
		void BadClusterHubId (
			std::string const & badHub, 
			std::string const & goodHub, 
			std::string const & project,
			std::string const & memberId,
			Transport const & transport)
		{
			_out << "The hub email (hub ID), \"" << badHub << "\", provided by a satellite member at "
				<< transport.GetRoute () << " is different from this hub's real email (hub ID), "
				<< goodHub << std::endl;
			_out << "This is a member of the project " << project << " with member id "
				<< memberId << std::endl;
			_out << "    FIX: Go to satellite machine, double click the dispatcher icon, "
				<< "and modify \"Hub's email address (or name)\" if different from " << goodHub
				<< std::endl;
			_out << "    Next: visit project " << project 
				<< " on that satellite and accept the change.\n";
			_out << "    FIX: If the project doesn't exist on the satellite\n"
				<< "    open Dispatcher>View Diagnostics, double-click the project name "
				<< "in the Members tab\n"
				<< "    and delete the entry in question.\n\n";
		}
		void NonTrivialHubTransport (std::string const hubId, Transport t)
		{
			_out << "Warning: Advanced feature used.\n";
			_out << "    Your Hub ID \"" << hubId << "\" not equal to hub transport\n"
				<< "    (" << t.GetMethodName () << " -> " << t.GetRoute () << ")\n";
			_out << "    Hub ID and transport are editable through Dispatcher Settings "
				<< "(to edit transport, press the Advanced button).\n\n";
		}
		void NonTrivialRemoteHubTransport (
											std::string const hubId, 
											Transport t, 
											Tree::Node const * node)
		{
			_out << "Warning: Advanced feature used.\n";
			_out << "    Remote Hub ID \"" << hubId << "\" not equal to hub transport\n"
				<< "    (" << t.GetMethodName () << " -> " << t.GetRoute () << ")\n";
			_out << "    These settings are editable on the remote hub machine.\n\n";
			node->PrintOut (4, _out);
			_out << "\n";
		}
		void UnknownHubTransport (std::string const hubId, Tree::Node const * node)
		{
			_out << "Warning: Transport unknown for this location: \"" << hubId << "\"\n";
			_out << "    FIX: You should probably remove this user from project\n"
				<< "    (or ask the administrator to do it).\n";
			node->PrintOut (4, _out);
			_out << "\n";
		}
	private:
		std::stringstream _out;
	};

	class Db
	{
	public:
		Db (ErrorLog & errorLog) 
			: _log (errorLog),
			  _local ("Local member(s)->Projects->User IDs"),
			  _cluster ("Cluster Members->Projects->User IDs"),
		      _nonLocal ("Hubs->Members->Projects->User IDs"),
			  _satellites ("Satellite path->Member->Project->User IDs")
		{}
		void SetMyHubId (std::string const & hubId) { _myHubId = hubId; }
		void SetTopology (Topology t) { _topology = t; }
		void SetHubRemoteTransport (Transport t)
		{
			if (!IsNocaseEqual (t.GetRoute (), _myHubId))
				_log.NonTrivialHubTransport (_myHubId, t);
		}

		void SetMyLocalTransport (Transport myLocalTransport)
		{
			// satellite to me
		}

		void Add (
			Enlistment const & enlistment,
			std::string const & hub, 
			std::string const & user, 
			std::string const & memberId,
			bool isMyself)
		{
			Tree::Node * rootNode = 0;
			Tree::Node * userNode = 0;
			if (isMyself)
			{
				if (!IsNocaseEqual (hub, _myHubId))
				{
					_log.BadLocalHubId (_myHubId, hub, enlistment, user, memberId);
				}
				rootNode = &_local;
			}
			else if (IsNocaseEqual (hub, _myHubId))
			{
				rootNode = &_cluster;
			}
			else
			{
				rootNode = _nonLocal.FindChild (hub);
				if (rootNode == 0)
					rootNode = _nonLocal.AddChild (hub);
			}

			userNode = rootNode->FindChild (user);
			if (userNode == 0)
				userNode = rootNode->AddChild (user);
			Tree::Node * projNode = userNode->FindChild (enlistment._projName);
			if (projNode == 0)
			{
				std::unique_ptr<Tree::Node> child (new Tree::ListNode (enlistment._projName));
				projNode = userNode->AddChild (std::move(child));
			}
			// duplicates possible (more than one local enlistment)
			if (!projNode->FindChild (memberId))
				projNode->AddChild (memberId);
		}

		void AddClusterRecipient (Address const & address, Transport const & transport)
		{
			if (!IsNocaseEqual (address.GetHubId (), _myHubId))
			{
				_log.BadClusterHubId (address.GetHubId (), 
									  _myHubId,
									  address.GetProjectName (),
									  address.GetUserId (),
									  transport);
				return;
			}
			// Find this user in the cluster tree
			// Note: we don't know the user name
			Tree::Node * userNode0 = 0;
			for (Tree::ConstChildIter it = _cluster.BeginChild ();
				it != _cluster.EndChild (); 
				++it)
			{
				Tree::Node * projNode0 = (*it)->FindChild (address.GetProjectName ());
				if (projNode0)
				{
					userNode0 = *it;
					break;
				}
			}
			// If we can't find the local record, it means the enlistment
			// exists on the satellite machine only
			std::string userName = userNode0? userNode0->GetName (): "[Satellite Only]";

			// Now insert it into the cluster tree
			Tree::Node * satNode = _satellites.FindChild (transport.GetRoute ());
			if (satNode == 0)
				satNode = _satellites.AddChild (transport.GetRoute ());
			Tree::Node * userNode = satNode->FindChild (userName);
			if (userNode == 0)
				userNode = satNode->AddChild (userName);
			Tree::Node * projNode = userNode->FindChild (address.GetProjectName ());
			if (projNode == 0)
			{
				std::unique_ptr<Tree::Node> child (new Tree::ListNode (address.GetProjectName ()));
				projNode = userNode->AddChild (std::move(child));
			}
			projNode->AddChild (address.GetUserId ());
		}
		bool empty () const { return true; }
		void Print (std::ostream & out);
		Tree::ConstChildIter BeginHub () const { return _nonLocal.BeginChild (); }
		Tree::ConstChildIter EndHub () const { return _nonLocal.EndChild (); }
	private:
		ErrorLog & _log;
		typedef std::pair<std::string, std::string> EnlistmentMemberIds;

		std::string _myHubId;
		Topology	_topology;

		Tree::Node _local;		// local machine - myself
		Tree::Node _cluster;	// with hub id == my hub id
		Tree::Node _nonLocal;	// other hubs
		Tree::Node _satellites;	// organized by computer
	};
}

void Membership::Db::Print (std::ostream & out)
{
	_local.Sort ();
	_local.PrintOut (0, out);

	if (_topology.HasSat ())
	{
		out << std::endl;
		_satellites.Sort ();
		_satellites.PrintOut (0, out);
	}
	if (_topology.HasHub ())
	{
		out << std::endl;
		_cluster.Sort ();
		_cluster.PrintOut (0, out);
	}
	out << std::endl;
	_nonLocal.Sort ();
	_nonLocal.PrintOut (0, out);
	out << std::endl;
}

//-------------------------------------
// Putting together diagnostic database
//-------------------------------------
void ScanDiagFile (char const * diagFile, int & bestLicenseProjId, Membership::Db & db)
{
	std::ifstream in (diagFile);
	XML::Scanner scanner (in);
	XML::Tree membershipTree;
	XML::TreeMaker maker (membershipTree);
	XML::Parser parser (scanner, maker);
	parser.Parse ();

	int bestSeatCount = 0;
	// Visit all membership nodes
	XML::Node const * root = membershipTree.GetRoot ();
	for (XML::Node::ConstChildIter iter = root->FirstChild (); iter != root->LastChild (); ++iter)
	{
		XML::Node const * node = *iter;
		if (node->GetName () == "Enlistment")
		{
			std::string const & projectId = node->GetAttribValue ("localId");
			XML::Node const * project = node->FindFirstChildNamed ("Project");
			std::string const & projectName = project->GetAttribValue ("name");
			Membership::Enlistment enlistment (projectName, projectId);

			XML::Node const * membership = node->FindFirstChildNamed ("Membership");
			if (membership != 0)
			{
				std::string const & myself = membership->GetAttribValue ("myself");
				for (XML::Node::ConstChildIter iter1 = membership->FirstChild (); 
					iter1 != membership->LastChild (); 
					++iter1)
				{
					XML::Node const * member = *iter1;
					std::string const & memberId = member->GetAttribValue ("id");
					std::string const & memberName = member->GetAttribValue ("name");
					std::string const & hubId = member->GetAttribValue ("hubId");
					bool isMyself = (memberId == myself);
					XML::Node const * state = member->FindFirstChildNamed ("State");
					if (state == 0 || state->GetAttribValue ("status") == "Defected")
						continue;
					db.Add (enlistment, hubId, memberName, memberId, isMyself);
					if (isMyself)
					{
						XML::Node const * license = member->FindFirstChildNamed ("License");
						if (license != 0)
						{
							// Found project owner license
							int version = ToInt (license->GetAttribValue ("version"));
							if (::IsCurrentVersion (version))
							{
								int seatCount = ToInt (license->GetAttribValue ("seats"));
								if (seatCount > bestSeatCount)
								{
									bestLicenseProjId = ToInt (projectId);
									bestSeatCount = seatCount;
								}
							}
						}
					}
				}
			}
		}
	}
}

void DumpMembership (char const * diagFile, char const * errFile, bool pickLicense)
{
	Membership::ErrorLog errorLog;
	Membership::Db db (errorLog);

	Catalog catalog;

	Topology myTopology = catalog.GetMyTopology ();
	db.SetTopology (myTopology);
	db.SetMyHubId (catalog.GetHubId ());

	int bestLicenseProjId = -1;
	ScanDiagFile (diagFile, bestLicenseProjId, db);

	// If we are a hub, list satellite machines
	// keyed on forwarding path
	if (myTopology.HasSat ())
	{
		for (SimpleClusterRecipSeq seq (catalog); !seq.AtEnd (); seq.Advance ())
		{
			Address address;
			Transport transport;
			bool isRemoved;

			seq.GetClusterRecipient (address, transport, isRemoved);
			if (!isRemoved)
				db.AddClusterRecipient (address, transport);
		}
	}

	Transport myPublicTransport = catalog.GetHubRemoteActiveTransport ();
	db.SetHubRemoteTransport (myPublicTransport);

	Transport myLocalTransport = catalog.GetActiveIntraClusterTransportToMe (); // my satellite to me
	db.SetMyLocalTransport (myLocalTransport);

	for (Tree::ConstChildIter it = db.BeginHub (); it != db.EndHub (); ++it)
	{
		std::string const & hubId = (*it)->GetName ();
		if (IsNocaseEqual (hubId, "anonymous"))
			continue;
		Transport hubTransport = catalog.GetInterClusterTransport (hubId);
		if (hubTransport.GetMethod () == Transport::Unknown)
			hubTransport.Init (hubId);
		if (hubTransport.GetMethod () == Transport::Unknown)
		{
			errorLog.UnknownHubTransport (hubId, *it);
		}
		else if (!IsNocaseEqual (hubId, hubTransport.GetRoute ()))
		{
			errorLog.NonTrivialRemoteHubTransport (hubId, hubTransport, *it);
		}
	}

	if (pickLicense && bestLicenseProjId != -1)
	{
		// Propagate the best license to the global database
		SccProxyEx sccProxy;
		sccProxy.CoopCmd (bestLicenseProjId,
						  "Maintenance PropagateLocalLicense:\"yes\"",
						  false,// Skip GUI Co-op in the project
						  true);// Execute command without timeout
	}


	std::string errors (errorLog.GetLog ());
	if (errFile != 0 && !errors.empty ())
	{
		std::ofstream err (errFile);
		err << "Diagnostics has discovered potential problems in your configuration:\n\n";
		err << errors;
		err << "\n\n--------------------------\n";
		err << "Full diagnostics dump follows:\n\n";
		db.Print (err);
	}
}
