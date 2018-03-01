#if !defined (LISTER_H)
#define LISTER_H
// (c) Reliable Software 2003
#include "DataPortal.h"
#include "DataQuery.h"

// These active objects own thir queries, have access to data sinks

// Lists a directory
class Lister: public Data::Provider
{
public:
	Lister (Data::ListQuery const & query, Data::Sink * sink, int id);
protected:
    void Run ();
	void Detach () { Data::Provider::Detach (); }
	void FlushThread () {}
private:
	Data::ListQuery _query;
};

// Lists drives
class DriveLister: public Data::Provider
{
public:
	DriveLister (Data::Sink * sink, int id);
protected:
    void Run ();
	void Detach () { Data::Provider::Detach (); }
	void FlushThread () {}
};

// Accumulates data from a lister
class Accumulator: public Data::Pipe
{
public:
	Accumulator (Data::ListQuery const & query, Data::Sink * sink, int id);
	// Data::Sink
	void DataReady (Data::ChunkHolder data, bool isDone, int srcId);
protected:
    void Run ();
	void Detach () { Data::Provider::Detach (); }
	void FlushThread () { _event.Release (); }
private:
	void MergeChunks ();
private:
	Win::Event			_event;
	Data::ListQuery		_query;
	// Chunk generator
	auto_active<Lister> _lister;

	Data::ChunkBuffer	_sources;
	Data::ChunkHolder	_prevResult;   // source for merge, also used by sink
	Data::ChunkHolder	_curResult;    // merge output
};

// Combines data from two queries (has two accumulators)
class Combiner: public Data::Pipe
{
public:
	Combiner (Data::CmpQuery const & query, Data::Sink * sink, int id);
	// Data::Sink
	void DataReady (Data::ChunkHolder data, bool isDone, int srcId);
protected:
    void Run ();
	void Detach () { Data::Provider::Detach (); }
	void FlushThread () { _event.Release (); }
private:
	void MergeChunks ();
private:
	Win::Event _event;

	auto_active<Lister>	_lister1;
	auto_active<Lister>	_lister2;

	Data::ChunkBuffer		_sources1;
	Data::ChunkBuffer		_sources2;

	Data::ChunkHolder _prevResult;
	Data::ChunkHolder _curResult;
};

class ContentComparator: public Data::Pipe
{
	enum State { stReady, stBusy, stFinishing };
public:
	ContentComparator (Data::CmpQuery const & query, Data::Sink * sink, int id);
	// Data::Sink
	void DataReady (Data::ChunkHolder data, bool isDone, int srcId);
protected:
    void Run ();
	void Detach () { Data::Provider::Detach (); }
	void FlushThread () { _event.Release (); }
private:
	void ProcessChunks ();
private:
	Data::CmpQuery		_query;
	State				_state;
	Win::Event			_event;
	Data::ChunkBuffer	_sources;
};

class Comparator: public Data::Pipe
{
	enum State { stReady = 0, stSourceEnd, stMergeEnd };
public:
	Comparator (Data::CmpQuery const & query, Data::Sink * sink, int id);
	// Data::Sink
	void DataReady (Data::ChunkHolder data, bool isDone, int srcId);
protected:
    void Run ();
	void FlushThread () { _event.Release (); }
private:
	void ProcessChunks ();
private:
	Win::Event			_event;
	State				_state;
	// combiner is a source of combined, accumulated data from two source, old and new
	auto_active<Combiner>			_combiner;
	// content comparator is used for items that are present in both old and new sources
	auto_active<ContentComparator>	_contentComparator;

	Data::ChunkBuffer	_sources;
	Data::ChunkHolder	_prevResult;   // source for merge, also used by sink
	Data::ChunkHolder	_curResult;    // merge output
};

#endif
