#if !defined VISITOR_H
#define VISITOR_H

//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "GlobalId.h"
#include "Area.h"

class DataBase;
class Path;
class SynchArea;
class PathFinder;
class UniqueName;
namespace Progress
{
	class Meter;
}

class Visitor
{
public:
    virtual ~Visitor ()
    {}

    virtual bool Visit (UniqueName const & uname, GlobalId & folderId) { return false; }
    virtual void Visit (GlobalId gid) {}
	virtual bool Visit (char const * path) { return false; }
	virtual void CancelVisit () {}
};

class Verifier: public Visitor
{
public:
    Verifier (DataBase const & database, PathFinder & pathFinder) 
        : _dataBase (database),
          _pathFinder (pathFinder)
    {}

protected:
    DataBase const & _dataBase;
    PathFinder &     _pathFinder;
};

class OriginalVerifier : public Verifier
{
public:
    OriginalVerifier (DataBase const & database, PathFinder & pathFinder) 
        : Verifier (database, pathFinder)
    {}

    void Visit (GlobalId gid);
};

class OriginalBackupVerifier : public Verifier
{
public:
    OriginalBackupVerifier (DataBase const & database, PathFinder & pathFinder)
        : Verifier (database, pathFinder)
    {}

    void Visit (GlobalId gid);
};

class SynchVerifier : public Verifier
{
public:
    SynchVerifier (DataBase const & database, PathFinder & pathFinder, SynchArea const & synchArea)
        : Verifier (database, pathFinder),
          _synchArea (synchArea)
    {}

    void Visit (GlobalId gid);

private:
    SynchArea const & _synchArea;
};

class ReferenceVerifier : public Verifier
{
public:
    ReferenceVerifier (DataBase const & database, PathFinder & pathFinder, SynchArea const & synchArea)
        : Verifier (database, pathFinder),
          _synchArea (synchArea)
    {}

    void Visit (GlobalId gid);

private:
    SynchArea const & _synchArea;
};

class StagingVerifier : public Verifier
{
public:
    StagingVerifier (DataBase const & database, PathFinder & pathFinder, bool redo)
        : Verifier (database, pathFinder), _redo (redo)
    {}

    void Visit (GlobalId gid);
private:
	bool	_redo;
};

class ProjectVerifier : public Verifier
{
public:
    ProjectVerifier (DataBase const & database, PathFinder & pathFinder, Progress::Meter & meter)
        : Verifier (database, pathFinder),
		  _meter (meter)
    {}

    bool Visit (UniqueName const & uname, GlobalId & folderId);

private:
	Progress::Meter &	_meter;
};

class PreSynchVerifier : public Verifier
{
public:
    PreSynchVerifier (DataBase const & database, PathFinder & pathFinder)
        : Verifier (database, pathFinder)
    {}

    void Visit (GlobalId gid);
};

class TemporaryVerifier : public Verifier
{
public:
    TemporaryVerifier (DataBase const & database, PathFinder & pathFinder)
        : Verifier (database, pathFinder)
    {}

    void Visit (GlobalId gid);
};

class CompareVerifier : public Verifier
{
public:
    CompareVerifier (DataBase const & database, PathFinder & pathFinder)
        : Verifier (database, pathFinder)
    {}

    void Visit (GlobalId gid);
};

#endif
