#if !defined (DISPATCHERIPC_H)
#define DISPATCHERIPC_H
//------------------------------------
//  (c) Reliable Software, 2002
//------------------------------------
#include "Serialize.h"

const int DISPATCHER_IPC_VERSION = 1;

class DispatcherIpcHeader: public Serializable
{
public:
	DispatcherIpcHeader (long flags = 0)
		: _flags (flags), _size (0), _version (DISPATCHER_IPC_VERSION)
	{}
	DispatcherIpcHeader (Deserializer& in, int version)
	{
		Deserialize (in, version);
	}
	long GetSize () const { return _size; }
	void SetSize (long size) { _size = size; }
	long GetFlags () const { return _flags; }

	int  GetVersion () const { return _version; }

	virtual void Serialize (Serializer& out) const
	{
		out.PutLong (_version);
		out.PutLong (_size);
		out.PutLong (_flags);
	}
	virtual void Deserialize (Deserializer& in, int version)
	{
		_version = in.GetLong ();
		_size = in.GetLong ();
		_flags = in.GetLong ();
	}
private:
	long _version;
	long _size;
	long _flags;
};

#endif
