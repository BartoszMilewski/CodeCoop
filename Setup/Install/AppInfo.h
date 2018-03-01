#if !defined (APPINFO_H)
#define APPINFO_H
//------------------------------------
//
//  (c) Reliable Software, 1997
//------------------------------------

//	currently unused
#if 0

#include <File/Path.h>

class AppInformation
{
public:
    AppInformation ()
        : _programPath ()
    {}

	//	if InitProgramPath is called on the global object it could look like a
	//	memory leak when the progam exits - add a helper class if needed (see
	//	co-op40/common/appinfo.h for example)
    void InitProgramPath (char const * cmdLine);

    char const * GetSetupFileSourcePath (char const * fileName) { return _programPath.GetFilePath (fileName); }

private:
    FilePath _programPath;
};

extern AppInformation TheAppInfo;

#endif

#endif
