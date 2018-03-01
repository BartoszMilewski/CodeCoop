#if !defined (CMDLINEMAKER_H)
#define CMDLINEMAKER_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

class PhysicalFile;
namespace XML { class Tree; }

class CmdLineMaker
{
public:
	virtual void MakeGUICmdLine (std::string & cmdLine, std::string & appPath) = 0;
	bool HasValue (std::string const & key) const;
protected:
	void TranslateArgs (std::string & cmdLine);
protected:
	typedef std::vector<std::pair<std::string, std::string> > ArgList;
	typedef std::vector<std::pair<std::string, std::string> >::const_iterator ArgIter;

	ArgList	_args;
};

class AltEditorCmdLineMaker : public CmdLineMaker
{
public:
	AltEditorCmdLineMaker (XML::Tree const & xmlArgs);

	void MakeGUICmdLine (std::string & cmdLine, std::string & appPath);
};

class AltDifferCmdLineMaker : public CmdLineMaker
{
public:
	AltDifferCmdLineMaker (XML::Tree const & xmlArgs);

	void MakeGUICmdLine (std::string & cmdLine, std::string & appPath);
};

class AltMergerCmdLineMaker : public CmdLineMaker
{
public:
	AltMergerCmdLineMaker (PhysicalFile const & file);
	AltMergerCmdLineMaker (XML::Tree const & xmlArgs);

	void MakeGUICmdLine (std::string & cmdLine, std::string & appPath);
	void MakeAutoCmdLine (std::string & cmdLine, std::string & appPath);
};

#endif
