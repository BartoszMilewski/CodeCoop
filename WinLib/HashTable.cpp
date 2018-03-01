//--------------------------------
// (c) Reliable Software 1997-2003
//--------------------------------
#include <WinLibBase.h>
#include "HashTable.h"
#include <StringOp.h>

using namespace Hash;

// The implementation of the hash table

// Find the list in hash table that may contain
// the id of the string we are looking for

IList & Table::Find (const char* str, int len) const
{
    int i = hash (str, len);
    Assert ( i >= 0 && i < _size );
    return _aList[i];
}

void Table::Add (int id, const char* str, int len)
{
    int i = hash (str, len);
    Assert (i >= 0 && i < _size);
	ILink * link = _storage.AllocLink (id);
    _aList[i].push_front (link);
}

// Private hashing function
// Disregards whitespace

int Table::hash (const char* str, int len) const
{
    // must be unsigned, hash should return positive number
    unsigned h = 0;
    for (int i = 0; i < len; i++)
    {
		char c = str[i];		
        if (!::IsSpace (c))
            h = (h << 4) + c;
    }
    return h % _size;  // small positive integer
}

ILink * LinkAlloc::AllocLink (int value)
{
	if (_curBlock == _storage.size ())
	{
		auto_array<ILink> block (new ILink [BLOCK_SIZE]);
		_storage.push_back (block);
	}

	ILink * newLink = & _storage [_curBlock] [_curLink];
	++_curLink;
	if (_curLink == BLOCK_SIZE)
	{
		++_curBlock;
		_curLink = 0;
	}
	newLink->SetValue (value);
	return newLink;
}

void LinkAlloc::Reset ()
{
	_curBlock = 0;
	_curLink = 0;
}

void IList::push_front (ILink * link)
{
	link->SetNext (_head);
	_head = link;
}

int IList::count ()
{
	int i = 0;
	ILink* link = _head;
	while (link != 0)
	{
		i++;
		link = link->GetNext();
	}
	return i;
}

