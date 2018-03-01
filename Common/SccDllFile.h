#if !defined (SCCDLLFILE_H)
#define SCCDLLFILE_H
//-----------------------------------------
//  SccDllFile.h
//  (c) Reliable Software, 1999
//-----------------------------------------

#include <File/Path.h>

class SccDllFile
{
public:
    SccDllFile ();

    char const * GetDllPath () const { return _programPath.GetFilePath (SccDllName); }

private:
    static char const SccDllName [];
    FilePath	_programPath;
};

#endif
