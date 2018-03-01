#if !defined (MAILBOX_H)
#define MAILBOX_H
//----------------------------------
// (c) Reliable Software 1998 - 2002
// ---------------------------------

#include "ScriptStatus.h"
#include "ScriptFileList.h"
#include <File/Path.h>
#include <File/Dir.h>

class MailTruck;
class WorkQueue;
class TransportData;
class DispatcherCmdExecutor;

class Mailbox
{
public:
    enum Status
    {
        stOk,
        stInvalidPath,
        stNotAccessible
    };
public:
    Mailbox (FilePath const & path, DispatcherCmdExecutor & cmdExecutor);
	Mailbox (Mailbox const & src);
	virtual ~Mailbox () {}
    
	void DeleteScript (std::string const & scriptName);
	void DeleteScriptAllChunks (std::string const & scriptName);
    bool UpdateFromDisk (NocaseSet & deletedScripts);

    virtual bool ProcessScripts (MailTruck & truck, WorkQueue & workQueue) = 0;

    Status GetStatus () const { return _status; }
    void ChangePath (FilePath const & newPath);
    char const * GetDir () const { return _path.GetDir (); };
    FilePath const & GetPath () const { return _path; };
    
    int GetScriptCount () const { return _scriptFiles.size (); }
	bool IsEmpty () const { return _scriptFiles.size () == 0; }

	void GetScriptFiles (std::vector<std::string> & filenames) const; // does not reset result vector/ 
	// Consider returning a limited interface
	ScriptFileList & GetScriptFileList() { return _scriptFiles; }
	std::unique_ptr<TransportData> GetTransportData (std::string const & scriptFilename) const;
	std::string GetScriptComment (std::string const & name) const;
	ScriptStatus GetScriptStatus (std::string const & name) const;
private:
    void ExamineState ();
protected:

    FilePath		_path;
    ScriptFileList	_scriptFiles;
    Status			_status;
	DispatcherCmdExecutor & _cmdExecutor;
	FileMultiSeq::Patterns _scriptPattern;
};

#endif
