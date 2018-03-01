//--------------------------------
// (c) Reliable Software 2002-2008
// -------------------------------

#include <WinLibBase.h>
#include "Help.h"
#include <HtmlHelp.h>

namespace Help
{
	bool Module::DisplayTopic(char const *topic, Win::Dow::Handle winParent)
	{
		if (winParent.IsNull ())
			winParent = ::GetDesktopWindow();
		//	HtmlHelp wants the topic parameter in the following format:
		//	module-name::\topic-name or
		//	module-name::\topic-name>window-name (for named windows)

		std::string arg = _moduleName;
		arg += "::\\";
		arg += topic;
		//	arg += ">";
		//	arg += windowName;

		Win::Dow::Handle hwnd = HtmlHelp (  winParent.ToNative (), 
											arg.c_str(),
											HH_DISPLAY_TOPIC, 
											0);
		return !hwnd.IsNull();
	}

	std::string Module::GetTopicPath(char const *topic)
	{
		//	The external path form of a topic is
		//  mk:@MSITStore:filepath::/topic
		//	where filepath is the typical C:\Dir\File.chm and
		//	topic is the internal name like Page.htm or Other\Page.htm
		std::string topicPath = "mk:@MSITStore:";
		topicPath += _moduleName;
		topicPath += "::/";
		topicPath += topic;
		return topicPath;
	}
}
