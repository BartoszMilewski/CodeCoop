#if !defined (MEMENTO_H)
#define MEMENTO_H
//
// (c) Reliable Software 1997 -- 2005
//

class Memento
{
};

class Reversable
{
public:
	virtual ~Reversable () {}

	virtual std::unique_ptr<Memento> CreateMemento () = 0;
	virtual void RevertTo (Memento const & memento) = 0;
	virtual void Clear () = 0;
};

#endif
