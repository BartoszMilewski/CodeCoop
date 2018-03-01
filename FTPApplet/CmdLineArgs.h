#if !defined (CMDLINEARGS_H)
#define CMDLINEARGS_H
//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

class CmdLineArgs
{
public:
	CmdLineArgs (char const * args);

	bool IsUpload () const;
	bool IsDownload () const;
	bool IsFileUpload () const;
	bool IsFileDownload () const;
	bool IsNoCleanup() const;

	std::string const & GetLocalPath () const;
	std::string const & GetRemotePath () const;

	std::string const & GetServer () const;
	std::string const & GetUser () const;
	std::string const & GetPassword () const;

private:
	std::string const & GetString (unsigned idx) const;

private:
	std::vector<std::string>	_args;
	std::string					_empty;
};

#endif
