#if !defined (IPCEXCHANGE_H)
#define IPCEXCHANGE_H
//-----------------------------------
//  (c) Reliable Software 2001 - 2006
//-----------------------------------

#include "Serialize.h"
#include "GlobalId.h"
#include "RandomUniqueName.h"

#include <Sys/Synchro.h>
#include <Sys/SharedMem.h>
#include <iosfwd>

// To use as a stream, create 
// std::ostream out (&xmlBuf);
class XmlBuf: public std::streambuf
{
public:
	XmlBuf ();
	XmlBuf (unsigned int handle);
	unsigned int GetHandle () const { return _sharedMem.GetHandle (); }
	char * GetBuf () { return _data; }
	unsigned GetBufSize () const { return DefaultBufSize; }
	enum
	{
		DefaultBufSize = 4096	// 4kb
	};
	// streambuf
	int sync ();
	int overflow (int ch = EOF);
	int underflow ();

private:
	SharedMem			_sharedMem;
	char *				_data;		// Primary memory buffer
	RandomUniqueName	_name;		// Exchange name
};

class IpcExchangeBuf : public SharedMem
{
public:
	IpcExchangeBuf ();
	IpcExchangeBuf (std::string const & name);
	IpcExchangeBuf (unsigned int handle);

	void MakeInvitation (int projectId, std::string const & name);
	void MakeAck (std::string const & info);
	void MakeErrorReport (std::string const & errMsg);
	void MakeCmd (std::string const & cmd, bool lastCmd);
	void MakeStateRequest ();
	void MakeVersionIdRequest (bool isCurrent);
	void MakeReportRequest (GlobalId versionGid);
	void MakeForkIdsRequest (GidList const & forkIds, bool deepForks);
	void MakeTargetPathRequest (GlobalId gid, std::string const & path);

	bool IsInvitation () const { return *_type == invitation; }
	bool IsAck () const { return *_type == acknowledgement; }
	bool IsErrorMsg () const { return *_type == errorMsg; }
	bool IsCmd () const { return *_type == command; }
	bool IsDataRequest () const { return *_type == request; }
	bool IsLast () const { return _flags->IsLast (); }
	bool IsStateRequest () const { return _flags->IsStateRequest (); }
	bool IsVersionIdRequest () const { return _flags->IsVersionIdRequest (); }
	bool IsCurrent () const { return _flags->IsCurrent (); }
	bool IsReportRequest () const { return _flags->IsReportRequest (); }
	bool IsForkIdsRequest () const { return _flags->IsForkIdsRequest (); }
	bool IsTargetPathRequest () const { return _flags->IsTargetPathRequest (); }
	bool IsDeppForkRequest () const { return _flags->IsDeepForkRequest (); }

	unsigned char * AllocBuf (unsigned int size);
	unsigned char const * GetReadData () { return GetBuf (); }
	unsigned char * GetWriteData (unsigned int size) { return AllocBuf (size); }
	unsigned int GetBufSize ();
	unsigned long GetBufferId () const { return _name.GetValue (); }
	std::string const & GetBufferName () const { return _name.GetString (); }

private:
	void Fill (std::string const & str);
	unsigned char * GetBuf ();
	unsigned int HeaderSize () const { return sizeof (Type) + sizeof (Flags); }
	unsigned int PrimaryCapacity () const { return DefaultBufSize - HeaderSize (); }
	void OpenExtraBuf ();

private:
#if !defined (NDEBUG)
	void DumpTypeAndFlags () const;
#endif

	class Flags
	{
	public:
		void SetExtraBuf (bool flag)			{ _bits._E = flag; }
		void SetLast (bool flag)				{ _bits._L = flag; }
		void SetStateRequest (bool flag)		{ _bits._S = flag; }
		void SetVersionIdRequest (bool flag)	{ _bits._V = flag; }

		void SetCurrent (bool flag)				{ _bits._C = flag; }
		void SetReportRequest (bool flag)		{ _bits._R = flag; }
		void SetForkIdsRequest (bool flag)		{ _bits._F = flag; }
		void SetTargetPathRequest (bool flag)	{ _bits._T = flag; }
		void SetDeepForks (bool flag)			{ _bits._D = flag; }

		bool IsExtraBuf () const				{ return _bits._E != 0; }
		bool IsLast () const					{ return _bits._L != 0; }
		bool IsStateRequest () const			{ return _bits._S != 0; }
		bool IsVersionIdRequest () const		{ return _bits._V != 0; }
		bool IsCurrent () const					{ return _bits._C != 0; }
		bool IsReportRequest () const			{ return _bits._R != 0; }
		bool IsForkIdsRequest () const			{ return _bits._F != 0; }
		bool IsTargetPathRequest () const		{ return _bits._T != 0; }
		bool IsDeepForkRequest () const			{ return _bits._D != 0; }

		void Clear () { _value = 0; }

	private:
		union
		{
			unsigned long _value;
			struct
			{
				unsigned long _E:1;     // Extra buffer used
				unsigned long _L:1;		// Last command or request
				unsigned long _S:1;		// File state request
				unsigned long _V:1;		// Version id request
				unsigned long _C:1;		// Current
				unsigned long _R:1;		// Report request
				unsigned long _F:1;		// Fork ids request
				unsigned long _T:1;		// Target path and status request
				unsigned long _D:1;		// Return deep fork ids
			} _bits;
		};
	};

	class ExtraBufId : public Serializable
	{
	public:
		ExtraBufId (unsigned int size, std::string const & name)
			: _size (size),
			  _name (name)
		{}
		ExtraBufId (Deserializer& in, int version = 0)
		{
			Deserialize (in, version);
		}

		int GetSize () const { return _size; }
		std::string const & GetName () const { return _name; }

		void Serialize (Serializer& out) const;
		void Deserialize (Deserializer& in, int version);

	private:
		unsigned int	_size;
		std::string		_name;
	};

	enum Type
	{
		unknown = 0,
		invitation,
		acknowledgement,
		command,
		request,
		differArgs,
		errorMsg
	};

	enum
	{
		DefaultBufSize = 4096	// 4kb
	};

private:
	Flags *						_flags;
	Type *						_type;		// Contents type
	unsigned char *				_data;		// Primary memory buffer
	std::unique_ptr<SharedMem>	_extraBuf;	// Optional extra buffer
	RandomUniqueName			_name;		// Exchange name
};

class IpcInvitation : public Serializable
{
public:
	IpcInvitation (int projId, std::string const & eventName)
		: _projId (projId),
		  _eventName (eventName)
	{}
	IpcInvitation (Deserializer& in, int version = 0)
	{
		Deserialize (in, version);
	}

	int GetProjId () const { return _projId; }
	std::string const & GetEventName () const { return _eventName; }

    void Serialize (Serializer& out) const;
    void Deserialize (Deserializer& in, int version);

private:
	int			_projId;
	std::string	_eventName;
};

#endif
