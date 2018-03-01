#if !defined (BITSDOWNLOADER_H)
#define BITSDOWNLOADER_H
// ----------------------------------
// (c) Reliable Software, 2003 - 2007
// ----------------------------------

#include "Download.h"
#include "BitService.h"

#include <Com/Com.h>

namespace BITS
{
	class Downloader : public ::Downloader
	{
	public:
		Downloader (std::string const & jobName, 
			CopyProgressSink & sink, 
			std::string const & serverName);
		
		bool IsAvailable () const { return _bits.get () != 0; }
		unsigned int CountDownloads ();

		void StartGetFile (std::string const & sourcePath,
						   std::string const & targetPath);
		bool Continue (std::string const & localFile);

		void CancelCurrent ();
		void CancelAll ();

		void SetTransientErrorIsFatal (bool isFatal);
		void SetForegroundPriority ();
		void SetNormalPriority ();
	private:
		std::string							  _url;
		std::string							  _jobName;
		Com::IfacePtr<BITS::StatusCallbacks>  _callbacks;
		std::unique_ptr<BITS::Service>		  _bits;
		std::unique_ptr<BITS::Job>		      _job;
	};
}

#endif
