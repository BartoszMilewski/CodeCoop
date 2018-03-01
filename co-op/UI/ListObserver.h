#if !defined (LISTOBSERVER_H)
#define LISTOBSERVER_H
//------------------------------------
//  (c) Reliable Software, 2005 - 2006
//------------------------------------

class ListObserver
{
public:
	virtual ~ListObserver () {}

	virtual void OnItemChange () = 0;
};

#endif
