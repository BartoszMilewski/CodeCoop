#if !defined (SCRIPTMANAGER_H)
#define SCRIPTMANAGER_H
//----------------------------------
//  (c) Reliable Software, 2009
//----------------------------------
class ScriptTicket;

class ScriptManager
{
public:
	virtual void ForgetScript(std::string const & scriptName) = 0;
	virtual void FinishScript(std::unique_ptr<ScriptTicket> script) = 0;
	virtual void ResetTimeout() = 0;
};

#endif
