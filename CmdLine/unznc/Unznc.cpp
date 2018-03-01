//----------------------------------
// (c) Reliable Software 2001 - 2009
//----------------------------------

#include "precompiled.h"
#include "CmdArgs.h"
#include "NamedBlock.h"
#include "Decompressor.h"
#include "Catalog.h"
#include "CmdLineVersionLabel.h"

#include <Ex/WinEx.h>
#include <Ex/Error.h>
#include <File/MemFile.h>
#include <File/Path.h>
#include <Ctrl/Output.h>

#include <iostream>

class UnzncSwitch : public StandardSwitch
{
public:
	UnzncSwitch ()
	{
		SetListing ();
	}
};

void UncompressScript (char const * fromPath, FilePath const & toPath, bool listingOnly);

int main (int count, char * args [])
{
	int retCode = 0;
	try
	{
		UnzncSwitch validSwitch;
		CmdArgs cmdArgs (count, args, validSwitch);
		if (cmdArgs.IsHelp () || cmdArgs.IsEmpty ())
		{
			std::cout << CMDLINE_VERSION_LABEL << ".\n\n";
			std::cout << "Uncompress packed scripts and copy them to the Public Inbox\n\n";
			std::cout << "unznc <options> <files>\n";
			std::cout << "options:\n";
			std::cout << "   -l -- only list compressed file names\n";
			std::cout << "   -? -- display help\n";
			return retCode;
		}

		Catalog catalog;
		FilePath toPath (catalog.GetPublicInboxDir ());
		if (!cmdArgs.IsListing ())
		{
			if (toPath.IsDirStrEmpty ())
				throw Win::Exception ("Cannot find the Public Inbox folder.");
			if (!File::Exists (toPath.GetDir ()))
				throw Win::Exception ("The Public Inbox folder doesn't exists -- cannot unpack scripts.", toPath.GetDir ());
		}

		char const ** inputFilePaths = cmdArgs.GetFilePaths ();
		for (unsigned int i = 0; i < cmdArgs.Size (); ++i)
		{
			PathSplitter splitter (inputFilePaths [i]);
			std::string fileName = splitter.GetFileName ();
			fileName += splitter.GetExtension ();
			if (IsFileNameEqual (splitter.GetExtension (), ".snc") ||
				IsFileNameEqual (splitter.GetExtension (), ".cnk"))
			{
				// Move uncompressed script to the Public Inbox
				File::Move (inputFilePaths [i], toPath.GetFilePath (fileName.c_str ()));
			}
			else
			{
				if (IsFileNameEqual (fileName, "chunk.znc"))
				{
					std::stringstream msg;
					msg << inputFilePaths [i] << std::endl;
					msg << "is part of a script that has been split into multiple parts." << std::endl << std::endl;
					msg << "Such scripts can only be uncompressed by the Code Co-op Dispatcher." << std::endl << std::endl;
					msg << "Contact support@ReliSoft.com if you require additional assistance.";
					Out::Sink sink;
					sink.Init (0, "Code Co-op Script Uncompression Problem");
					sink.Display (msg.str ().c_str (), Out::Error);
				}
				else
				{
					// Uncompress packed scripts and copy them to the Public Inbox
					UncompressScript (inputFilePaths [i], toPath, cmdArgs.IsListing ());
				}
			}
		}
	}
	catch (Win::Exception e)
	{
		std::cerr << "unznc: " << e.GetMessage () << std::endl;
		SysMsg msg (e.GetError ());
		if (msg)
			std::cerr << "System tells us: " << msg.Text ();
		std::string objectName (e.GetObjectName ());
		if (!objectName.empty ())
			std::cerr << "    " << objectName << std::endl;
		retCode = 1;
	}
	catch (...)
	{
		Win::ClearError ();
		std::cerr << "unznc: Unknown problem" << std::endl;
		retCode = 1;
	}
	return retCode;
}

void UncompressScript (char const * fromPath, FilePath const & toPath, bool listingOnly)
{
	TmpPath tmpPath;
	std::unique_ptr<OutNamedBlock> firstBlock;
	if (listingOnly)
		firstBlock.reset (new MemOutBlock ());
	else
		firstBlock.reset (new FileOutBlock (tmpPath));
	OutBlockVector unpackedFiles (firstBlock);

	{
		MemFileReadOnly packedFile (fromPath);
		Decompressor decompressor;
		unsigned char const * buf = reinterpret_cast<unsigned char const *>(packedFile.GetBuf ());
		File::Size size = packedFile.GetSize ();
		if (!decompressor.Decompress (buf, size, unpackedFiles))
			throw Win::Exception ("Corrupted compressed script file -- cannot unpack scripts.");
	}

	if (listingOnly)
	{
		std::cout << fromPath  << std::endl;
		std::cout << "Contains the following script files:" << std::endl;
	}
	else
	{
		std::cout << "Unpacking scripts:" << std::endl;
	}

	for (unsigned int j = 0; j < unpackedFiles.size (); ++j)
	{
		OutNamedBlock & curBlock = unpackedFiles [j];
		if (!listingOnly)
			curBlock.SaveTo (toPath);
		std::cout << "   " << curBlock.GetName () << std::endl;
	}

	if (!listingOnly)
		File::DeleteNoEx (fromPath);
}
