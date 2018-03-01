#if !defined (PROXY_H)
#define PROXY_H
//---------------------------------------
//  Proxy.h
//  (c) Reliable Software, 1998 -- 2002
//---------------------------------------

#include "Global.h"

#include <Sys/Process.h>

class CoopProxy : public Win::ProcessProxy
{
public:
	CoopProxy ()
		: Win::ProcessProxy (CoopClassName)
	{}
};

class DispatcherProxy : public Win::ProcessProxy
{
public:
	DispatcherProxy ()
		: Win::ProcessProxy (DispatcherClassName)
	{}
};

class DifferProxy : public Win::ProcessProxy
{
public:
	DifferProxy ()
		: Win::ProcessProxy (DifferClassName)
	{}
};

#endif
