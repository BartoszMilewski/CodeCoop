#if !defined BlockFinder_h
#define BlockFinder_h

#include <HashTable.h>
#include <File/File.h>

class  MatchingBlock
{
public :
	MatchingBlock () {}
	MatchingBlock (int source, int destination, int len)
		:_source (source), _destination (destination), _len (len)
	{}
	int DestinationOffset () const {return _destination;}
	int SourceOffset () const { return _source;}
	int Len () const { return _len;}
	void SetNewParametrs (int source, int destination, int len)
	{
		_source = source;
		_destination = destination;
		_len = len;
	}

private :
	int _destination;
	int _source;
	int _len;
public :
	// Predidatcts
	class GreaterLen
	{
	public:
		bool operator () (const MatchingBlock & gr, const MatchingBlock & sm)
		{
			return gr.Len () > sm.Len ();		
		}
	};

	class LessDestination
	{
	public:
		bool operator () (const MatchingBlock & sm, const MatchingBlock  & gr)
		{		
				return (gr.DestinationOffset () > sm.DestinationOffset ());
		}
	};

	class LessSource
	{
	public:
		bool operator () (const MatchingBlock & sm, const MatchingBlock & gr)
		{
			return (gr.SourceOffset () > sm.SourceOffset ());
		}
	};

	class LessThan
	{
	public:
		LessThan (const MatchingBlock & pBlock) 
			: _len (pBlock.Len ()) 
		{}
		bool operator () (const MatchingBlock  & sm)
		{
			return (sm.Len () < _len);
		}
	private:
		int	_len;
	};
};

class GapsFinder
{
public:
	GapsFinder (File::Size sizeOldFile, File::Size sizeNewFile, std::vector<MatchingBlock> & blocks)
		: _oldBufSize (sizeOldFile.Low ()), _newBufSize (sizeNewFile.Low ()), _blocks (blocks)
	{}
	void GetAddBlocks (std::vector<MatchingBlock> & gaps);
	void GetDeleteBlocks (std::vector<MatchingBlock> & gaps);
private:
	int const _oldBufSize;
	int const _newBufSize;
    std::vector<MatchingBlock> & _blocks;
};

class ListIt : public Hash::ListIterC
{
public :
	ListIt (Hash::IList const & list)
		: Hash::ListIterC (list)
	{}
};

class HashTable
{	
public :
	HashTable (int size)
		:_size (size)
	{
		_lists = new Hash::IList [_size];
	}	
	~HashTable ()	
	{
		delete []_lists;
	}
	void Reset ()
	{
		_storage.Reset ();
		for (int k = 0; k < _size; ++k)
			_lists [k].clear ();
	}
	ListIt GetList (unsigned long hashnumber)
	{ 
		return ListIt (_lists [hashnumber % _size]);
	}
    void AddLink (int value, unsigned long hashnumber)
	{
		Hash::ILink * link = _storage.AllocLink (value);
        _lists [hashnumber % _size].push_front (link);
	} 

private :
	Hash::IList *    _lists;
	Hash::LinkAlloc	 _storage;	
	const int        _size;
};

class FreeSpaceRecorder
{
public :
	FreeSpaceRecorder (int sizeOldFile, int sizeNewFile)
		: _isOldFree(sizeOldFile, true), _isNewFree(sizeNewFile, true)
	{}
	void Reset ();
	void CutBlock (MatchingBlock & block);
	void SetSpaceInUse (const MatchingBlock & block);
private :
	std::vector<bool> _isOldFree;
	std::vector<bool> _isNewFree;
};

class BlockFinder
{
private :
	enum Paramerts 
	{ 
		minBlockSize = 7
	};
public :
	BlockFinder (char const * const oldFile, File::Size sizeOldFile,
				 char const * const newFile, File::Size sizeNewFile);
	void Find (std::vector<MatchingBlock> & blocks);
private:	
	void HashOldFile ();
	void ChooseMarkers (std::list<char> & markers);
	void ScanNewFile ();
	void FindMatchingBlocks ();
	void RefineClusters ();
	int LeftMatch (int indexOld, int indexNew) const;
	int RightMatch (int indexOld, int indexNew) const;
	unsigned long NewHashAround (int indexNew) const;
	unsigned long OldHashAround (int indexOld) const;
	int DuplicationLen (int index) const;
	void FindEasyClusters ();

	const File::Size			_sizeOldFile;
	const char * const			_oldFile;
	int	const					_oldBufSize;

	const File::Size			_sizeNewFile;
	const char * const			_newFile;
	int	const					_newBufSize;

	char						_currentMarkerChar;	
	HashTable                   _hashTable;
	std::vector<MatchingBlock>  _recorder;
	FreeSpaceRecorder           _freeSpace;
};
#endif
