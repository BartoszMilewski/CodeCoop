#if !defined (COOPEXEC_H)
#define COOPEXEC_H
//------------------------------------
//  (c) Reliable Software, 2000 - 2007
//------------------------------------

class CoopExec
{
public:
	CoopExec ();

	bool Start ();
	bool StartServer (std::string const & bufferName, unsigned int keepAliveTimeout, bool stayInProject);

private:
	bool Execute (std::string const & cmd, bool showNormal);

private:
	std::string	_coopPath;
};

#endif
