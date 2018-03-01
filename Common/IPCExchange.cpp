//-----------------------------------
//  (c) Reliable Software 2001 - 2006
//-----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "IpcExchange.h"
#include "SerString.h"
#include "SerVector.h"

#include <Dbg/Out.h>
#include <Dbg/Assert.h>

// Create new shared buffer
XmlBuf::XmlBuf ()
{
	_sharedMem.Create (DefaultBufSize, _name.GetString ());
	_data = reinterpret_cast<char *> (_sharedMem.GetRawBuf ());
	setp (_data, _data + DefaultBufSize);
	setg (_data, _data, _data + DefaultBufSize);
}

// Opens existing shared buffer
XmlBuf::XmlBuf (unsigned int handle)
{
	_sharedMem.Map (handle);
	_data = reinterpret_cast<char *> (_sharedMem.GetRawBuf ());
	setp (_data, _data + DefaultBufSize);
	setg (_data, _data, _data + DefaultBufSize);
}

// streambuf interface
int XmlBuf::sync ()
{
	char * pch = pptr();
	Assert (pch != 0);
	if (pch - _data >= DefaultBufSize)
		return -1;
	return 0;
}

int XmlBuf::overflow (int ch)
{
	return EOF;
}

int XmlBuf::underflow ()
{
	return  gptr() == egptr() ?
			traits_type::eof() :
			traits_type::to_int_type(*gptr());
}

// Create new shared buffer
IpcExchangeBuf::IpcExchangeBuf ()
{
	Create (DefaultBufSize, _name.GetString ());
	unsigned char * p = GetRawBuf ();
	_type = reinterpret_cast<Type *> (p);
	p += sizeof (Type);
	_flags = reinterpret_cast<Flags *> (p);
	p += sizeof (Flags);
	_data = p;
	Assert (_data == GetRawBuf () + HeaderSize ());
	*_type = unknown;
	_flags->Clear ();
	dbg << "    Creating IPC exchange buffer: name = " << _name.GetString ().c_str () << std::endl;
}

// Opens existing shared buffer
IpcExchangeBuf::IpcExchangeBuf (std::string const & name)
	: SharedMem (name, DefaultBufSize)
{
	dbg << "    Opening IPC exchange buffer: name = " << name.c_str () << std::endl;
	unsigned char * p = GetRawBuf ();
	_type = reinterpret_cast<Type *> (p);
	p += sizeof (Type);
	_flags = reinterpret_cast<Flags *> (p);
	p += sizeof (Flags);
	_data = p;
	Assert (_data == GetRawBuf () + HeaderSize ());
#if !defined (NDEBUG)
	DumpTypeAndFlags ();
#endif
	if (_flags->IsExtraBuf ())
	{
		// Extra buffer used
		OpenExtraBuf ();
	}
}

// Opens existing shared buffer
IpcExchangeBuf::IpcExchangeBuf (unsigned int handle)
{
	dbg << "    Opening IPC exchange buffer" << std::endl;
	Map (handle);
	unsigned char * p = GetRawBuf ();
	_type = reinterpret_cast<Type *> (p);
	p += sizeof (Type);
	_flags = reinterpret_cast<Flags *> (p);
	p += sizeof (Flags);
	_data = p;
	Assert (_data == GetRawBuf () + HeaderSize ());
#if !defined (NDEBUG)
	DumpTypeAndFlags ();
#endif
	if (_flags->IsExtraBuf ())
	{
		// Extra buffer used
		OpenExtraBuf ();
	}
}

void IpcExchangeBuf::Fill (std::string const & str)
{
	unsigned int len = str.length ();
	unsigned char * buf = AllocBuf (len + 1);
	MemorySerializer out (buf, len + 1);
	out.PutBytes (str.c_str (), len);
	out.PutByte (0);
}

unsigned char * IpcExchangeBuf::AllocBuf (unsigned int size)
{
	if (size < PrimaryCapacity ())
	{
		// Use primary buffer
		_flags->SetExtraBuf (false);
		return _data;
	}
	else
	{
		// Extra buffer has to be allocated
		_extraBuf.reset (new SharedMem (size));
		_flags->SetExtraBuf (true);
		dbg << "    Allocating extra buffer: " << _extraBuf->GetName () << "; size: " << size << std::endl;
		ExtraBufId id (size, _extraBuf->GetName ());
		MemorySerializer out (_data, PrimaryCapacity ());
		id.Serialize (out);
		return _extraBuf->GetRawBuf ();
	}
}

void IpcExchangeBuf::MakeInvitation (int projectId, std::string const & name)
{
	*_type = unknown;
	Assert (name.length () + sizeof (long) + 1 < PrimaryCapacity ());
	IpcInvitation ipcInvitation (projectId, name);
	MemorySerializer out (_data, PrimaryCapacity ());
	ipcInvitation.Serialize (out);
	*_type = invitation;
	dbg << "    Making invitation -- project id = " << projectId << std::endl;
}

void IpcExchangeBuf::MakeAck (std::string const & info)
{
	*_type = unknown;
	if (!info.empty ())
		Fill (info);
	*_type = acknowledgement;
}

void IpcExchangeBuf::MakeErrorReport (std::string const & errMsg)
{
	*_type = unknown;
	if (!errMsg.empty ())
		Fill (errMsg);
	*_type = errorMsg;
}

void IpcExchangeBuf::MakeCmd (std::string const & cmd, bool lastCmd)
{
	*_type = unknown;
	Fill (cmd);
	_flags->SetLast (lastCmd);
	*_type = command;
}

void IpcExchangeBuf::MakeStateRequest ()
{
	*_type = unknown;
	_flags->SetStateRequest (true);
	_flags->SetLast (true);
	*_type = request;
}

void IpcExchangeBuf::MakeVersionIdRequest (bool isCurrent)
{
	*_type = unknown;
	_flags->SetVersionIdRequest (true);
	if (isCurrent)
		_flags->SetCurrent (true);
	else
		_flags->SetCurrent (false);
	_flags->SetLast (true);
	*_type = request;
	Assert (IsDataRequest ());
}


void IpcExchangeBuf::MakeReportRequest (GlobalId versionGid)
{
	*_type = unknown;
	_flags->SetReportRequest (true);
	_flags->SetLast (true);
	MemorySerializer out (AllocBuf (sizeof (GlobalId)), sizeof (GlobalId));
	out.PutLong (versionGid);
	*_type = request;
}

void IpcExchangeBuf::MakeForkIdsRequest (GidList const & forkIds, bool deepForks)
{
	*_type = unknown;
	_flags->SetForkIdsRequest (true);
	_flags->SetDeepForks (deepForks);
	_flags->SetLast (true);
	CountingSerializer counter;
	SerVector<GlobalId> list (forkIds);
	list.Serialize (counter);
	Assert (!LargeInteger(counter.GetSize ()).IsLarge ());
	unsigned int size = static_cast<unsigned>(counter.GetSize ());
	MemorySerializer out (AllocBuf (size), size);
	list.Serialize (out);
	*_type = request;
}

void IpcExchangeBuf::MakeTargetPathRequest (GlobalId gid, std::string const & path)
{
	*_type = unknown;
	_flags->SetTargetPathRequest (true);
	_flags->SetLast (true);
	CountingSerializer counter;
	counter.PutLong (gid);
	SerString str (path);
	str.Serialize (counter);
	Assert (!LargeInteger(counter.GetSize ()).IsLarge ());
	unsigned int size = static_cast<unsigned>(counter.GetSize ());
	MemorySerializer out (AllocBuf (size), size);
	out.PutLong (gid);
	str.Serialize (out);
	*_type = request;
}

unsigned char * IpcExchangeBuf::GetBuf ()
{
	if (_flags->IsExtraBuf ())
	{
		if (_extraBuf.get () == 0)
		{
			OpenExtraBuf ();
		}
		return _extraBuf->GetRawBuf ();
	}
	else
	{
		return _data;
	}
}

unsigned int IpcExchangeBuf::GetBufSize () 
{ 
	if (_flags->IsExtraBuf ())
	{
		if (_extraBuf.get () == 0)
		{
			OpenExtraBuf ();
		}
		return _extraBuf->GetSize ();
	}
	else
	{
		return PrimaryCapacity ();
	}
}

void IpcExchangeBuf::OpenExtraBuf ()
{
	MemoryDeserializer in (_data);
	ExtraBufId id (in);
	dbg << "    Opening extra buffer: " << id.GetName () << "; size: " << id.GetSize () << std::endl;
	_extraBuf.reset (new SharedMem (id.GetName (), id.GetSize ()));
}

#if !defined (NDEBUG)
void IpcExchangeBuf::DumpTypeAndFlags () const
{
	dbg << "    Type: ";
	 switch (*_type)
	 {
	 case unknown:
	 	dbg << "unknown";
	 	break;
	 case invitation:
	 	dbg << "invitation";
	 	break;
	 case acknowledgement:
	 	dbg << "acknowledgement";
	 	break;
	 case command:
	 	dbg << "commnad";
	 	break;
	 case request:
	 	dbg << "request";
	 	break;
	 case differArgs:
	 	dbg << "differ arguments";
	 	break;
	 case errorMsg:
	 	dbg << "error message";
	 	break;
	 default:
	 	dbg << "illegal";
	 	break;
	 }
	 dbg << "; Flags: ";
	 if (_flags->IsExtraBuf ())
	 	dbg << "Extra buf used;";
	 if (_flags->IsLast ())
	 	dbg << " Last command;";
	 if (_flags->IsStateRequest ())
	 	dbg << " State request";
	 if (_flags->IsReportRequest ())
		dbg << " Report request";
	 dbg << std::endl;
}
#endif

void IpcExchangeBuf::ExtraBufId::Serialize (Serializer & out) const
{
	out.PutLong (_size);
	unsigned int len = _name.length ();
	out.PutLong (len);
	if (len > 0)
		out.PutBytes (_name.data (), len);
}

void IpcExchangeBuf::ExtraBufId::Deserialize (Deserializer & in, int version)
{
	_size = in.GetLong ();
	unsigned int len = in.GetLong ();
	if (len > 0)
	{
		_name.resize (len);
		char * buf = &_name[0];
        in.GetBytes (buf, len);
	}
}

void IpcInvitation::Serialize (Serializer & out) const
{
	out.PutLong (_projId);
	unsigned int len = _eventName.length ();
	out.PutLong (len);
	if (len > 0)
		out.PutBytes (_eventName.data (), len);
}

void IpcInvitation::Deserialize (Deserializer & in, int version)
{
	_projId = in.GetLong ();
	unsigned int len = in.GetLong ();
	if (len > 0)
	{
		_eventName.resize (len);
		char * buf = &_eventName[0];
        in.GetBytes (buf, len);
	}
}
