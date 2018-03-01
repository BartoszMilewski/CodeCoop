#if !defined (LOCK_H)
#define LOCK_H
//----------------------------------
//  (c) Reliable Software, 1998-2003
//----------------------------------

// To prevent reentrancy
class ReentranceLock
{
public:
	ReentranceLock (bool & isBusy)
		: _isBusy (isBusy) 
	{ 
		Assert (!_isBusy);
		_isBusy = true; 
	}
	~ReentranceLock () 
	{ 
		_isBusy = false; 
	}
private:
	bool & _isBusy;
};

#endif
