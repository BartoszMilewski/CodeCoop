#if !defined (APPHELP_H)
#define APPHELP_H
//----------------------------------
// (c) Reliable Software 2002 - 2007
// ---------------------------------

#include <Win/Win.h>

namespace AppHelp
{
	char const CodeCoopHelpFile [] = "CodeCoop.chm";
	char const ContentsTopic [] = "Help\\Overview.html";
	char const IndexTopic [] = "Help\\Command.html";
	char const TutorialTopic [] = "Tutorial\\tutorial.html";
	char const SupportTopic [] = "Help\\Support.html";
	char const DispatcherTopic [] = "Tutorial\\WizardStart.html";
	char const DifferTopic [] = "Help\\Differ.html";
	char const ProjectJoin [] = "Help\\JoinProject.html";
	char const MoveToMachine[] = "Help\\MoveProjects.html";
	char const OESecurity [] = "FAQs\\OESecurity.html";

	void Display(
		char const *topic, 
		char const *content, 
		Win::Dow::Handle winParent = Win::Dow::Handle ());
}

#endif
