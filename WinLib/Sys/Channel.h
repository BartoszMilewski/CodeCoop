#if !defined (CHANNEL_H)
#define CHANNEL_H
//---------------------------
// (c) Reliable Software 2010
//---------------------------
#include <Sys/Synchro.h>

//-----------------------------------------------
// Building blocks for inter-thread communication
// A channel is split into the synchronization part, ChannelSync,
// and the storage part, where messages are stored
//-----------------------------------------------

// The synchronization part of a channel
class ChannelSync
{
	friend class PutLock;
	friend class GetLock;
public:
	// false if timed out
	bool Wait(int timeout = Win::Event::NoTimeout)
	{
		return _event.Wait (timeout);
	}
	void Release()
	{
		_event.Release();
	}
protected:
	mutable Win::Mutex		_mutex;
	mutable Win::Event		_event;
};

// Helper class: PutLock
class PutLock: public Win::MutexLock
{
public:
	PutLock(ChannelSync & chan)
		: _chan(chan), Win::MutexLock(chan._mutex)
	{}
	~PutLock()
	{
		_chan._event.Release();
	}
private:
	ChannelSync & _chan;
};

// Helper class GetLock
class GetLock: public Win::MutexLock
{
public:
	GetLock(ChannelSync & chan)
		: Win::MutexLock(chan._mutex)
	{}
};

// This is an example of the storage part of a channel
// Its methods should be called under the channel lock

class BoolStore
{
public:
	BoolStore() : _flag(false) {}
	// Use PutLock to synchronize
	void Put()
	{
		_flag = true;
	}
	// Use GetLock to synchronize
	bool Get()
	{
		bool tmp = _flag;
		_flag = false;
		return tmp;
	}
	// Use GetLock to synchronize
	bool IsEmpty() const
	{
		return !_flag;
	}
private:
	bool _flag;
};

template<class T>
class ValueStore
{
public: 
	ValueStore()
		: _valid(false)
	{}
	// Use PutLock to synchronize
	void Put(T t)
	{
		_message = t;
		_valid = true;
	}
	// Use GetLock to synchronize
	T Get()
	{
		_valid = false;
		return _message;		
	}
	// Use GetLock to synchronize
	bool IsEmpty() const
	{
		return !_valid;
	}
private:
	T 		_message;
	bool 	_valid;
};

// The sender uses SendChannel built of a sync and a store
template<class T, template<class> class ChStore>
class SendChannel
{
public:
	SendChannel(ChannelSync & sync, ChStore<T> & store)
		:_sync(sync),
		_store(store)
	{}
	void Put(T msg)
	{
		PutLock lock(_sync);
		_store.Put(msg);
	}
private:
	ChannelSync & _sync;
	ChStore<T> & _store;
};

class BoolSendChannel
{
public:
	BoolSendChannel(ChannelSync & sync, BoolStore & store)
		: _sync(sync),
		  _store(store)
	{}
	void Put()
	{
		PutLock lock(_sync);
		_store.Put();
	}
private:
	ChannelSync & _sync;
	BoolStore & _store;
};

// Simple non-composable receive channel 
template<class T, template<class> class ChStore>
class RcvChannel
{
public:
	RcvChannel(ChannelSync & sync, ChStore<T> & store)
		:_sync(sync),
		_store(store)
	{}
	bool Wait(int timeout = Win::Event::NoTimeout)
	{
		return _sync.Wait(timeout);
	}
	T Get()
	{
		GetLock lock(_sync);
		T msg = _store.Get();
		return msg;
	}
private:
	ChannelSync & _sync;
	ChStore<T> & _store;
};


// Interface for wrapped channels
class WrappedChannel
{
public:
	// Must be called under GetLock
	virtual bool Peek() = 0;
	virtual WrappedChannel * Receive() = 0;
	// Called outside of the lock
	virtual void Execute() = 0;
};

// Wrapped channel combining value store and a message handler function
// Should be called under lock
template<class T, class Fun>
class WrappedValueChannel: public WrappedChannel
{
public:
	WrappedValueChannel(ValueStore<T> & store, Fun f)
		: _store(store), _f(f)
	{}
	// Should be called under lock
	WrappedChannel * Receive()
	{
		if (!_store.IsEmpty())
		{
			_copy = _store.Get();
			return this;
		}
		return 0;
	}
	bool Peek()
	{
		return !_store.IsEmpty();
	}
	void Execute() 
	{
		_f(_copy);
	};
private:
	ValueStore<T> & _store;
	Fun _f;
	T   _copy;
};

template<class Fun>
class BoolWrapper: public WrappedChannel
{
public:
	BoolWrapper(BoolStore & store, Fun & f)
		: _store(store), _f(f)
	{}
	WrappedChannel * Receive()
	{
		if (_store.Get())
			_f();
		return 0;
	}
	bool Peek()
	{
		return !_store.IsEmpty();
	}
	// Called outside of the lock
	void Execute()
	{}

private:
	BoolStore & _store;
	Fun & _f;
};

template<class Storage, class Handler>
BoolWrapper<Handler> CreateBoolWrapper(Storage s, Handler f)
{
	return BoolWrapper(s, f);
}

template<class Fun>
class BoolDelayWrapper: public WrappedChannel
{
public:
	BoolDelayWrapper(BoolStore & store, Fun & f)
		: _store(store), _f(f)
	{}
	WrappedChannel * Receive()
	{
		return _store.Get()? this: 0;
	}
	bool Peek()
	{
		return !_store.IsEmpty();
	}
	// Called outside of the lock
	void Execute()
	{
		_f();
	}
private:
	BoolStore & _store;
	Fun & _f;
	bool  _lastVal;
};

// The receiver's end of aggregate channels sharing the same sync
// Each channel is represented by its WrappedChannel--a combination 
// of storage and handler
class RcvMultiChannel
{
public:
	RcvMultiChannel(ChannelSync & sync)
		: _sync(sync)
	{}
	bool Wait(int timeout = Win::Event::NoTimeout)
	{
		return _sync.Wait(timeout);
	}
	void RegisterChannel(WrappedChannel * chan)
	{
		// assert that sender uses the same _schan
		_channels.push_back(chan);
	}
	void Receive()
	{
		std::vector<WrappedChannel*> execs;
		{
			GetLock lock(_sync);
			for(std::vector<WrappedChannel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
			{
				WrappedChannel * exec = (*it)->Receive();
				if (exec)
					execs.push_back(exec);
			}
			// end lock
		}
		for (std::vector<WrappedChannel*>::iterator it = execs.begin(); it != execs.end(); ++it)
			(*it)->Execute();
	}
	bool Peek()
	{
		GetLock lock(_sync);
		for(std::vector<WrappedChannel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
		{
			if ((*it)->Peek())
				return true;
		}
		return false;
	}
private:
	ChannelSync & _sync;
	std::vector<WrappedChannel *> _channels;
};

#if 0
// Test
class Fint
{
public:
	void operator()(int i) { TheOutput.Display("int received"); }
};

class Fstring
{
public:
	void operator()(std::string str) { TheOutput.Display(str.c_str()); }
};

class Communicator
{
public:
	Communicator()
	{
		WrappedValueChannel<int, Fint> wrapIntChan(_intStore, _fint);
		WrappedValueChannel<std::string, Fstring> wrapStrChan(_stringStore, _fstring);
		RcvMultiChannel rcv(_sync);
		Assert (!rcv.Peek());
		rcv.RegisterChannel(&wrapIntChan);
		rcv.RegisterChannel(&wrapStrChan);
		SendChannel<int, ValueStore> sndInt(_sync, _intStore);
		SendChannel<std::string, ValueStore> sndStr(_sync, _stringStore);
		//sndInt.Put(7);
		sndStr.Put("Hello");
		if (rcv.Wait())
		{
			rcv.Receive();
		}
	}
private:
	ChannelSync _sync;
	ValueStore<int> _intStore;
	Fint _fint;
	Fstring _fstring;
	ValueStore<std::string> _stringStore;
};
#endif
#endif
