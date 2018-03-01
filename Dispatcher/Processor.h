#if !defined (PROCESSOR_H)
#define PROCESSOR_H
//-----------------------------------
// (c) Reliable Software 1998 -- 2005
// ----------------------------------

#include "ScriptProcessorConfig.h"

class FilePath;
class SafePaths;

class ScriptProcessor
{
public:
	ScriptProcessor (ScriptProcessorConfig const & config)
		: _procConfig (config)
	{}

	std::string Pack (SafePaths & scriptPaths,
					  std::string const & scriptPath,
					  FilePath const & tmpFolder, 
					  std::string & extension,
					  std::string const & projectName) const;
    bool Unpack  (std::string const & fromPath, 
				  FilePath const & toPath, 
				  std::string const & extension,
				  std::string const & title); // for error reporting
	
	ScriptProcessorConfig const & GetConfig () const
	{
		return _procConfig;
	}
	static bool HasIgnoredExtension (std::string const & path);

private:
    class Extension
    {
    public:
        Extension (std::string const & extension);
        bool IsEqual (char const * ext) const;
        bool empty () const { return _ext.empty (); }

    private:
        std::string _ext;
    };

private:
    static bool IsIgnoredExtension (Extension const & ext);
	void Compress (SafePaths const & scriptPaths, std::string const & packedFilePath) const;
	void CompressOne (std::string const & srcPath, std::string const & packedFilePath) const;
	void Decompress (std::string const & packedFilePath, FilePath const & toPath, std::string const & title) const;
    void RunUserTool (char const * curFolder, std::string const & cmdLine) const;

private:
	static char	const 		_compressedFileName [];
	static char	const 		_compressedFileExt [];
	ScriptProcessorConfig	const & _procConfig;
};

#endif
