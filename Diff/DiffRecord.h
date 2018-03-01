#if !defined (DIFFRECORD_H)
#define DIFFRECORD_H
//
// Reliable Software (c) 1998 -- 2002
//

#include "CheckSum.h"
#include <File/File.h>

class SimpleLine;
class Cluster;

class DiffRecorder
{
public:
	virtual ~DiffRecorder () {}

	virtual void SetNewFileSize (File::Size size) = 0;
	virtual void SetOldFileSize (File::Size size) = 0;
    virtual void SetNewCheckSum (CheckSum checkSum) = 0;
    virtual void AddCluster (Cluster const & cluster) = 0;
    virtual void AddNewLine (int lineNo, SimpleLine const & line) = 0;
    virtual void AddOldLine (int lineNo, SimpleLine const & line) = 0;
};

#endif
