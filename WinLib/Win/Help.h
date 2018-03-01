#if !defined (HELP_H)
#define HELP_H
//------------------------------------------------
// Help.h
// (c) Reliable Software 2002
// -----------------------------------------------

namespace Help
{
	class Engine
	{
	public:
		virtual ~Engine () {}

		virtual bool OnDialogHelp (int dlgId) throw ()
			{ return false; }

	protected:
		Engine () {}
	};

	//	For use with .chm files
	class Module
	{
	public:
		Module(char const *moduleName) : _moduleName(moduleName) {}
		bool DisplayTopic(char const *topic, Win::Dow::Handle winParent = Win::Dow::Handle ());
		std::string GetTopicPath(char const *topic);

	private:
		std::string _moduleName;
	};
}

#endif
