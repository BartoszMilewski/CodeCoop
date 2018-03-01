//-------------------------------------
// (c) Reliable Software 1999, 2000, 01
// ------------------------------------

#include "precompiled.h"
#include "ForgetfulHashTable.h"

#include <Dbg/Assert.h>

ForgetfulHashTable::LinkAllocator ForgetfulHashTable::_linkAllocator;

ForgetfulHashTable::LinkAllocator::Block::Block (unsigned int itemSize)
{
	// Organize new memory Block as a linked list of items of size 'itemSize'
	const unsigned int itemCount = BlockSize / itemSize;
	char * last = &_mem [(itemCount - 1) * itemSize];
	Assert (sizeof (Link) <= itemSize);
	for (char * p = &_mem [0]; p < last; p += itemSize)
	{
		reinterpret_cast<Link *>(p)->_next = reinterpret_cast<Link *>(p + itemSize);
	}
	reinterpret_cast<Link *>(last)->_next = 0;
}

void ForgetfulHashTable::LinkAllocator::Free (PositionItem * list)
{
	if (list == 0)
		return;

	// Put back the list to the free elemets pool
	PositionItem * last = list;
	// Find list end
	while (last->Next () != 0)
		last = last->Next ();

	last->SetNext (_free);
	_free = list;
}

void ForgetfulHashTable::Clear ()
{
	_linkAllocator.Purge ();
	std::fill (_lists.begin (), _lists.end (), reinterpret_cast<PositionItem *>(0));
}
