#if !defined (DOWNLOAD_H)
#define DOWNLOAD_H
// ----------------------------------
// (c) Reliable Software, 2003 - 2007
// ----------------------------------

class FilePath;

class Downloader
{
public: 
	virtual ~Downloader () {}
	virtual bool IsAvailable () const = 0;
	virtual unsigned int CountDownloads () = 0;
	virtual void StartGetFile (std::string const & sourcePath,
							   std::string const & targetPath) = 0;
	virtual bool Continue (std::string const & localFile) = 0;
	virtual void CancelCurrent () = 0;
	virtual void CancelAll () = 0;
	
	virtual void SetTransientErrorIsFatal (bool isFatal) = 0;
	virtual void SetForegroundPriority () = 0;
	virtual void SetNormalPriority () = 0;
};

#endif
