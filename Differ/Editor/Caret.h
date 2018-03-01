#if !defined (CARET_H)
#define CARET_H
//
// (c) Reliable Software, 1997
//

#include <Win/Caret.h>

class ColMapper;

class Caret: public Win::Caret
{
public:
    Caret (Win::Dow::Handle win, ColMapper const & mapper) 
		: Win::Caret (win), _mapper (mapper) 
    {}
    void Create ();
    void Position (int col, int line);
	void GetPosition (int & col, int & line) const;
private:
    ColMapper const   & _mapper;
};

#endif
