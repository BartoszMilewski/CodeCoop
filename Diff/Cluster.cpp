//------------------------------------
// (c) Reliable Software 1997
//------------------------------------
#include "precompiled.h"
#include "Cluster.h"
#include <Dbg\Assert.h>

Cluster Cluster::SplitCluster (int offset)
{
	int oldLine = OldLineNo () + offset;
	int newLine = NewLineNo ();
	if (newLine != -1)
		newLine += offset;
	int len = Len () - offset;
	Assert (len > 0);
	ShortenFromEnd (len);
	return Cluster (oldLine, newLine, len);
}

void Cluster::ExtendUp ()
{
	Assert (_oldLineNo > 0);
	Assert (_newLineNo > 0);
	--_oldLineNo;
	--_newLineNo;
	++_len;
}
