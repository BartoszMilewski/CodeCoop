//------------------------------------
//
//  (c) Reliable Software, 1997
//------------------------------------

#include "precompiled.h"

//	currently unused
#if 0

#include "AppInfo.h"
#include <Dbg/Assert.h>

AppInformation TheAppInfo;

void AppInformation::InitProgramPath (char const * cmdLine)
{
    // Get program path from command line
    int len = strlen (cmdLine);
    int lastSlash = -1;
    int start = 0;
    if (cmdLine [0] == '\"')
        start = 1;
    for (int i = start; i < len; i++)
    {
        if (cmdLine [i] == '\\')
            lastSlash = i;
        else if (cmdLine [i] == '\"')
            break;
    }
    Assert (lastSlash >= 0);
    int lenPrefix = lastSlash - start;
    Assert (lenPrefix < MAX_PATH);
    char pathBuf [MAX_PATH];
    memcpy (pathBuf, cmdLine + start, lenPrefix);
    pathBuf [lenPrefix] = '\0';
    _programPath.Change (pathBuf);
}

#endif