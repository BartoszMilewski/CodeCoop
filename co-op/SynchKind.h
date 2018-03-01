#if !defined (SYNCHKIND_H)
#define SYNCHKIND_H
//---------------------------------------
//  SynchKind.h
//  (c) Reliable Software, 2002
//---------------------------------------

enum SynchKind
{
	synchNew = 0,
	synchEdit = 1,
	synchDelete = 2,
	synchRename = 3,
	synchNone = 4,
	synchRestore = 5,
	synchMove = 6,
	synchRemove = 7,
	synchLastKind = 8
};

#endif
