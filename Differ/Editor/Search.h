#if !defined (SEARCH_H)
#define SEARCH_H
//-------------------------------------
// (c) Reliable Software 2001
//-------------------------------------

class SearchRequest
{
public :
	SearchRequest ();
	void SetFindWord (std::string const & word) { _findWord = word; }
	std::string const & GetFindWord () const { return _findWord; }
	std::string & GetFindWord () { return _findWord; }
	bool IsDirectionForward () const { return _directionForward; }
	bool IsDirectionBackward () const { return !_directionForward; }
	void SetDirectionForward (bool directionForward) { _directionForward = directionForward; } 
	bool IsMatchCase () const { return _matchCase;}
	bool IsWholeWord () const { return _wholeWord;}

protected :
	bool           _directionForward;
	bool           _matchCase;
	bool           _wholeWord;
	std::string    _findWord;

};

class ReplaceRequest : public virtual SearchRequest
{
public :
	std::string & GetSubstitution () { return _substitution; }
	std::string const & GetSubstitution () const { return _substitution;} 
protected :
	std::string _substitution;
};

#endif
