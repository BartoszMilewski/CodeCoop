#if !defined (VALIDATORS_H)
#define VALIDATORS_H
//----------------------------------
// (c) Reliable Software 2005
//----------------------------------

class ChunkSizeValidator
{
public:
	ChunkSizeValidator (unsigned chunkSize)
		: _chunkSize (chunkSize)
	{}

	bool IsInValidRange () const
	{ 
#if defined NDEBUG // in release version don't allow < 100k
		return (100 <= _chunkSize && _chunkSize <= 102400); 
#else
		return (10 <= _chunkSize && _chunkSize <= 102400); 
#endif
	}

	static unsigned GetDefaultChunkSize () { return 1024; }	// Default is 1Mb
	static std::string GetMinChunkSizeDisplayString () 
	{ 
#if defined NDEBUG // in release version don't allow < 100k
		return "100kb"; 
#else
		return "10kb"; 	
#endif
	}
	static std::string GetMaxChunkSizeDisplayString () { return "100Mb"; }

private:
	unsigned	_chunkSize;
};

class UpdatePeriodValidator
{
	static unsigned int const MinUpdatePeriod = 7;
	static unsigned int const MaxUpdatePeriod = 31;
	static unsigned int const DefaultUpdatePeriod = 7;
public:
	UpdatePeriodValidator (unsigned int period)
		: _period (period)
	{}
	bool IsValid () const { return _period >= MinUpdatePeriod && _period <= MaxUpdatePeriod; }
	void DisplayError () const;

	static unsigned int GetMax () { return MaxUpdatePeriod; }
	static unsigned int GetMin () { return MinUpdatePeriod; }
	static unsigned int GetDefault () { return DefaultUpdatePeriod; }
private:
	unsigned int _period;
};

class ScriptProcessorConfig;

class ScriptProcessorValidator
{
public:
	ScriptProcessorValidator (ScriptProcessorConfig const & cfg)
		: _cfg (cfg)
	{}
	bool IsValid () const;
	void DisplayErrors () const;
private:
	ScriptProcessorConfig const & _cfg;
};

#endif
