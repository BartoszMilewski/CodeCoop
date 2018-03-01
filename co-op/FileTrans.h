#if !defined (FILETRANS_H)
#define FILETRANS_H
//-----------------------------------
// (c) Reliable Software 1998 -- 2002
//-----------------------------------

#include "Transact.h"
#include "FileList.h"
#include <Ex/WinEx.h>

class FileTransaction : public Transaction
{
public:
	FileTransaction (Transactable& xable, SysPathFinder& pathFinder, TransactionFileList & fileList)
		: Transaction (xable, pathFinder),
		 _fileList (fileList)
	{}

	void Commit ()
	{
		PhaseOne ();
		// Now we can only commit or exit
		CommitOrDie ();
	}

private:
	void PhaseOne ()
	{
		_fileList.CommitPhaseOne ();
		CommitPhaseOne ();
	}

	void CommitOrDie () throw (Win::ExitException)
	{
		_fileList.CommitOrDie ();
		CommitPhaseTwo ();
	}

private:
	TransactionFileList & _fileList;
};

#endif
