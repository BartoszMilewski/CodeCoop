#if !defined (SYNCHTRANS_H)
#define SYNCHTRANS_H
//
// (c) Reliable Software 1997
//
#include "FileTrans.h"
class DataBase;
class PathFinder;

class SynchTransaction: public FileTransaction
{
public:
    SynchTransaction (Transactable& xable, 
		PathFinder& pathFinder, 
		DataBase& dataBase, 
		TransactionFileList & fileList);
};


#endif
