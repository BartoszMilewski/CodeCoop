#if !defined (DATAPORTAL_H)
#define DATAPORTAL_H
// Reliable Software (c) 2002
#include "DataChunk.h"
#include <Sys/Active.h>

namespace Data
{
	class Query
	{
	public:
		virtual ~Query () {}
		virtual bool operator== (Query const & query) { return false; }
	};

	// A sink accpepts data through thread-save DataReady callback
	// The provider is identified by an id
	class Sink
	{
	public:
		virtual ~Sink () {}
		virtual void DataReady (ChunkHolder data, bool isDone, int srcId) = 0;
		virtual void DataClear () {}
	protected:
		Win::CritSection _critSect;
	};

	// A provider is an active object that manufactures data
	// and deposits it in the sink. A provider is given an id

	class Provider: public ActiveObject
	{
	public:
		Provider (Sink * sink, int id)
			:_sink (sink), _id (id)
		{}
		virtual ~Provider () {}
		void Detach () { _sink = 0; }
		virtual void ResetSink (Sink * sink) { _sink = sink; }
		int GetId () const { return _id; }
	protected:
		Sink *		_sink;
		int const	_id;
	};

	// A Pipe is both
	class Pipe: public Sink, public Provider
	{
	public:
		Pipe (Sink * sink, int id)
			: Provider (sink, id)
		{}
		void Detach ()
		{
			Win::Lock lock (_critSect);
			Provider::Detach ();
		}
	};
}
#endif
