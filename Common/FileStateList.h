#if !defined (FILESTATELIST_H)
#define FILESTATELIST_H
//-----------------------------------------
//  FileStateList.h
//  (c) Reliable Software, 2000, 01
//-----------------------------------------

#include "FileState.h"

class FileStateList
{
public:
	FileStateList (unsigned char * buf)
	{
		Init (buf);
	}
	FileStateList (unsigned char const * buf)
	{
		Init (const_cast<unsigned char *>(buf));
	}

	bool empty () const { return _pathList.empty (); }

public:
	class Iterator
	{
	public:
		Iterator (FileStateList & stateList)
			: _cur (0),
			  _stateList (stateList.GetStateList ()),
			  _pathList (stateList.GetPathList ())
		{}

		void Advance () { ++_cur; }
		bool AtEnd () const { return _cur == _pathList.size (); }

		unsigned int GetState () const { return _stateList [_cur]; }
		char const * GetPath () const { return _pathList [_cur]; }

		void SetState (unsigned int state) { _stateList [_cur] = state; }

	private:
		unsigned int						_cur;
		unsigned int *						_stateList;
		std::vector<char const *> const &	_pathList;
	};

	class ConstIterator
	{
	public:
		ConstIterator (FileStateList const & stateList)
			: _cur (0),
			  _stateList (stateList.GetStateList ()),
			  _pathList (stateList.GetPathList ())
		{}

		void Advance () { ++_cur; }
		bool AtEnd () const { return _cur == _pathList.size (); }

		unsigned int GetState () const { return _stateList [_cur]; }
		char const * GetPath () const { return _pathList [_cur]; }

	private:
		unsigned int						_cur;
		unsigned int const *				_stateList;
		std::vector<char const *> const &	_pathList;
	};

	friend class FileStateList::Iterator;
	friend class FileStateList::ConstIterator;

private:
	void Init (unsigned char *);
	unsigned int * GetStateList () { return _stateList; }
	unsigned int const * GetStateList () const { return _stateList; }
	std::vector<char const *> const & GetPathList () const { return _pathList; }

private:
	unsigned int *				_stateList;
	std::vector<char const *>	_pathList;
};

#endif
