#if !defined UNDOFILES_H
#define UNDOFILE_H

//------------------------------------
// (c) Reliable Software 1998
//------------------------------------

class UndoTransferFiles
{
public:
	UndoTransferFiles ()
		: _commit (false)
	{}
	~UndoTransferFiles ();
	void RememberCreated (char const * fullPath, bool isFolder);
	void Commit ();

private:
	void Delete (std::vector<std::string> & fullPath, std::vector<bool> & isFolder);

	std::vector<std::string>	_created;
	std::vector<bool>	_createdIsFolder;

	bool				_commit;
};

#endif
