// (c) Reliable Software 2003
#include "precompiled.h"
#include "WinOut.h"
#include <Net/Ftp.h>
#include <Sys/Crypto.h>
#include <Sys/Synchro.h>

void GetPutTest (WinOut & output);
void AsynchronousTest (WinOut & output);
void AsyncListingTest (WinOut & output);
void ReadTest (WinOut & output);
void WriteTest (WinOut & output);
void DirTest (WinOut & output);
void TestEncryption (WinOut & output);

void RunAll (WinOut & output)
{
	output.PutLine ("Start test");
	// GetPutTest (output);
	// AsynchronousTest (output);
	// ReadTest (output);
	DirTest (output);
	// WriteTest (output);
	// TestEncryption (output);
	output.PutLine ("End test");
}

int RunTest (WinOut & winOut)
{
	int status = 0;
	try
	{
		RunAll (winOut);
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch (char const * msg)
	{
		TheOutput.Display (msg, Out::Error);
	}
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Display ("Internal error: Unknown exception", Out::Error);
	}
	return status;
}

void GetPutTest (WinOut & output)
{
	char url [] = "ftp://ftp.relisoft.com";
	char server [] = "ftp.relisoft.com";
	char tmpFile [] = "c:\\tmp\\version.xml";
	char versionFile [] = "version.xml";

	{
		Internet::Access access ("Code Co-op");
		if (!access.AttemptConnect ())
			throw Internet::Exception ("Internet connection failed");
		if (!access.CheckConnection (url, true))
			throw Internet::Exception ("Cannot connect", url);
		Internet::AutoHandle hInternet (access.Open ());
		Ftp::Session session (hInternet);
		session.SetServer (server);
		session.SetUser ("relisoft");
		session.SetPassword ("UkN3u1gW");
		Ftp::AutoHandle hFtp (session.Connect ());
		if (!File::Exists (tmpFile))
			throw Win::Exception ("You have to create a test file", tmpFile);
		if (!hFtp.DeleteFile (versionFile))
		{
			int err = Win::GetError ();
		}
		if (!hFtp.PutFile (tmpFile, versionFile))
			throw Internet::Exception ("Cannot put file", versionFile);
	}
	{
		Internet::Access access ("Code Co-op");
		Internet::AutoHandle hInternet (access.Open ());
		// anonymous FTP
		Ftp::Session session (hInternet);
		session.SetServer (server);
		Ftp::AutoHandle hFtp (session.Connect ());
		bool found = false;
		for (Ftp::FileSeq seq (hFtp, "*.*"); !seq.AtEnd (); seq.Advance ())
		{
			std::string name (seq.GetName ());
			if (name == versionFile)
			{
				found = true;
				break;
			}
		}
		if (!found)
			throw Win::Exception ("File not found on FTP server", server);
		if (!hFtp.GetFile (versionFile, tmpFile))
		{
			throw Internet::Exception ("Cannot get file", versionFile);
		}
	}
}

class Callback: public Internet::Callback
{
public:
	Callback (int n) : _n (n), _error (0), _done (false) {}
	unsigned GetError () { return _error; }
	void OnRequestComplete (Internet::Handle h, unsigned error)
	{
		_error = error;
		_done = true;
		_event.Release ();
	}
	void OnHandleCreated (Internet::Handle h, unsigned error)
	{
		_h = h;
		_event.Release ();
	}
	Internet::AutoHandle WaitForHandle ()
	{
		do
		{
			Wait ();
		} while (_h.IsNull () && _error == 0);

		if (_error)
			throw Internet::Exception ("Handle could not be created");
		return Internet::AutoHandle (_h.ToNative ());
	}
	void WaitForCompletion ()
	{
		do
		{
			Wait ();
		} while (!_done && _error == 0);
	}
private:
	void Wait ()
	{
		_event.Wait ();
	}

	int _n; // for debugging
	Win::Event _event;
	Internet::Handle _h;
	unsigned	_error;
	bool		_done;
};

void AsyncListingTest (WinOut & output)
{
	char server [] = "ftp.relisoft.com";
	Callback callback (2);

	Internet::Access access ("Code Co-op");
	access.WorkAsync ();
	Internet::AutoHandle hInternet (access.Open ());
	// anonymous FTP
	Ftp::Session session (hInternet);
	session.SetServer (server);
	session.Connect (Internet::FTP, &callback);
	Ftp::AutoHandle hFtp (callback.WaitForHandle ());

	{
		// use fresh callback
		Callback cBack (3);
		// ???? This doesn't work as expected ???
		Ftp::FileSeq seq (hFtp, "*.*", &cBack);
		Internet::AutoHandle hSeq = cBack.WaitForHandle ();
		cBack.WaitForCompletion ();
		for (; !seq.AtEnd (); seq.Advance ())
		{
			std::string name (seq.GetName ());
		}
	}
}

void AsynchronousTest (WinOut & output)
{
	char server [] = "ftp.relisoft.com";
	char tmpPath [] = "c:\\tmp\\co-op.exe";
	char largeFile [] = "co-op.exe";

	Callback callback (0);

	Internet::Access access ("Code Co-op");
	access.WorkAsync ();
	Internet::AutoHandle hInternet (access.Open ());
	// anonymous FTP
	Ftp::Session session (hInternet);
	session.SetServer (server);
	session.Connect (Internet::FTP, &callback);
	Ftp::AutoHandle hFtp (callback.WaitForHandle ());

	{
		// use fresh callback
		Callback callback (1);
		if (!hFtp.GetFile ( largeFile, 
							tmpPath, 
							File::NormalAttributes (), 
							&callback))
		{
			throw Internet::Exception ("Cannot get file", largeFile);
		}
		callback.WaitForCompletion ();
		if (callback.GetError () != 0)
			throw Internet::Exception ("wait failed", 0, callback.GetError ());
	}
}

void ReadTest (WinOut & output)
{
	char server [] = "ftp.relisoft.com";

	Internet::Access access ("Code Co-op");
	Internet::AutoHandle hInternet (access.Open ());
	// anonymous FTP
	Ftp::Session session (hInternet);
	session.SetServer (server);
	Ftp::AutoHandle hFtp (session.Connect ());
	{
		char tmpPath [] = "c:\\tmp\\co-op.exe";
		char largeFile [] = "co-op.exe";
		Ftp::FileReadable file (hFtp, largeFile);
	}
	{
		char versionFile [] = "version.xml";
		char tmpFile [] = "c:\\tmp\\version.xml";
		Ftp::FileReadable file (hFtp, versionFile);
		char buf [1024];
		unsigned long size;
		do
		{
			size = sizeof (buf);
			file.Read (buf, size);
		} while (size != 0);
	}
}

void WriteTest (WinOut & output)
{
	char server [] = "ftp.relisoft.com";
	Internet::Access access ("Code Co-op");
	output.PutLine ("Opening Internet");
	Internet::AutoHandle hInternet (access.Open ());
	Ftp::Session session (hInternet);
	session.SetServer (server);
	session.SetUser ("relisoft");
	session.SetPassword ("UkN3u1gW");
	char testFile [] = "test.txt";
	output.PutLine ("Connecting to FTP site");
	Ftp::AutoHandle hFtp (session.Connect ());
	{
		output.PutLine ("Creating a writable file");
		Ftp::FileWritable file (hFtp, testFile);
		char buf [] = "This is a test\r\n";
		unsigned long size = strlen (buf);
		output.PutLine ("Writing");
		output.PutLine (buf);
		file.Write (buf, size);
		Assert (size == strlen (buf));
		output.PutLine ("Writing");
		output.PutLine (buf);
		file.Write (buf, size);
		Assert (size == strlen (buf));
		output.PutLine ("Closing file");
	}
	output.PutLine ("Deleting file");
	hFtp.DeleteFile (testFile);
	output.PutLine ("Closing connection");
}

void DirTest (WinOut & output)
{
	char server [] = "ftp.relisoft.com";
	Internet::Access access ("Code Co-op");
	output.PutLine ("Opening Internet");
	Internet::AutoHandle hInternet (access.Open ());
	Ftp::Session session (hInternet);
	session.SetServer (server);
	session.SetUser ("relisoft");
	session.SetPassword ("UkN3u1gW");
	output.PutLine ("Connecting to FTP site");
	Ftp::AutoHandle hFtp (session.Connect ());

	{
		output.PutLine ("List current directory");
		Ftp::FileSeq seq (hFtp, "*.*");
		while (!seq.AtEnd ())
		{
			std::string name (seq.GetName ());
			output.PutLine (name.c_str ());
			seq.Advance ();
		}
	}
	output.PutLine ("Create directory");
	hFtp.CreateDirectory ("testDir");
	hFtp.SetCurrentDirectory ("testDir");
	char path [MAX_PATH];
	unsigned long len = sizeof (path);
	hFtp.GetCurrentDirectory (path, len);
	Assert (strcmp (path, "/testDir") == 0);
	hFtp.SetCurrentDirectory ("/");
	len = sizeof (path);
	hFtp.GetCurrentDirectory (path, len);
	output.PutLine ("Remove directory");
	bool result = hFtp.RemoveDirectory ("testDir");
	if (!result)
		throw Internet::Exception ("Cannot remove directory", "testDir");
	output.PutLine ("Closing connection");
}

void TestEncryption (WinOut & output)
{
	Crypt::String crypt ("destroy before reading", "test");
	output.PutLine (crypt.GetPlainText ());
	unsigned size;
	unsigned char const * data = crypt.GetData (size);
	output.PutLine ("Encrypted");
	Crypt::String decrypt (data, size);
	output.PutLine ("Decrypted");
	output.PutLine (decrypt.GetPlainText ());
	Assert (strcmp (crypt.GetPlainText (), decrypt.GetPlainText ()) == 0);
}


