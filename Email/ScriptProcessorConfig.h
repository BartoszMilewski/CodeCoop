#if !defined (SCRIPTPROCESSORCONFIG_H)
#define SCRIPTPROCESSORCONFIG_H
// ----------------------------------
// (c) Reliable Software, 2000 - 2008
// ----------------------------------

namespace Email { class Manager; }

class ScriptProcessorConfig
{
public:
	ScriptProcessorConfig ()
		: _preproNeedsProjName (false),
		  _canSendUnprocessed (false)
	{}
	bool NeedsUserPreprocessing () const 
	{ 
		return !_preproResult.empty () && !_preproCommand.empty (); 
	}
	void SetPreproCommand (std::string const & command)
	{
		_preproCommand = command;
	}
	void SetPostproCommand (std::string const & command)
	{
		_postproCommand = command;
	}
	void SetPreproResult (std::string const & resultFile)
	{
		_preproResult = resultFile;
	}
	void SetCanSendUnprocessed (bool canSendUnprocessed)
	{
		_canSendUnprocessed = canSendUnprocessed;
	}
	void SetPostproExt (std::string const & ext)
	{
		_postproExt = ext;
	}
	void SetPreproNeedsProjName (bool flag)
	{
		_preproNeedsProjName = flag;
	}

	char const * GetPreproCommand  () const { return _preproCommand.c_str (); }
	char const * GetPostproCommand () const { return _postproCommand.c_str (); }

 	char const * GetPreproResult () const { return _preproResult.c_str (); }
	char const * GetPostproExt   () const { return _postproExt.c_str (); }
	bool PreproNeedsProjName () const { return _preproNeedsProjName; }

	bool CanSendUnprocessed () const { return _canSendUnprocessed; }
	bool IsPostproExtEmpty    () const { return _postproExt.empty (); }

private:
	// before sending
	std::string _preproCommand;
	std::string _preproResult;
	bool		_preproNeedsProjName;
	bool		_canSendUnprocessed;
	// after receiving
	std::string _postproCommand;
	std::string _postproExt;
};

#endif
