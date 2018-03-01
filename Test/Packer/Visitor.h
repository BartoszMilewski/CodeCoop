#if !defined (VISITOR_H)
#define VISITOR_H
//-----------------------------------------------------
//  (c) Reliable Software 2001
//-----------------------------------------------------

class ProgressMeter;
class FilePath;

class Visitor
{
public:
    virtual ~Visitor ()
    {}

	virtual bool Visit (char const * path) { return false; }
};


class Traversal
{
public:
	Traversal (FilePath const & path, Visitor & visitor, ProgressMeter & progressMeter);

private:
	void TraverseTree (FilePath const & path, bool isRoot);

private:
    Visitor &		_visitor;
	ProgressMeter & _progressMeter;
};

#endif
