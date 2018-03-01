//-----------------------------------
// (c) Reliable Software 1998 -- 2006
// ----------------------------------

#include "precompiled.h"
#include "Processor.h"
#include "Serialize.h"
#include "TransportHeader.h"
#include "PackerGlobal.h"
#include "Compressor.h"
#include "Decompressor.h"
#include "NamedBlock.h"
#include "ScriptName.h"
#include "AlertMan.h"

#include <File/Path.h>
#include <File/File.h>
#include <File/SafePaths.h>
#include <Ex/WinEx.h>
#include <Ex/Error.h>
#include <Dbg/Assert.h>
#include <Com/Shell.h>
#include <Sys/Process.h>

char const ScriptProcessor::_compressedFileName [] = "Scripts.znc";
char const ScriptProcessor::_compressedFileExt [] = "znc";

bool ScriptProcessor::Unpack (std::string const & fromPath, 
							  FilePath const & toPath, 
							  std::string const & extension,
							  std::string const & title)
{
	// Revisit: there should be a better way of 
	// getting file name from path
	PathSplitter splitter (fromPath);
	std::string fromFolder = splitter.GetDrive ();
	fromFolder += splitter.GetDir ();

	std::string fileName = splitter.GetFileName ();
	std::string fileExt (splitter.GetExtension ());
	fileName += fileExt;

	bool ateExtension = false; // email client ate it! (Eudora?)
	if (!extension.empty () && fileExt.empty ())
	{
		ateExtension = true;
		fileExt = extension;	// Use attachement extension specified in the message subject
								// Otherwise use the attachment file extension
	}

	Extension ext (fileExt);
	if (IsIgnoredExtension (ext))
		return true;

	if (ext.empty () || ext.IsEqual ("dat"))
	{
		std::unique_ptr<TransportHeader> transHdr;
		try
		{
			FileDeserializer in (fromPath.c_str ());
			transHdr.reset (new TransportHeader (in));
		}
		catch (...) 
		{
			Win::ClearError ();
			transHdr.reset ();
		}
		
		if (transHdr.get () != 0 && !transHdr->IsEmpty ())
		{
			std::unique_ptr<ScriptSubHeader> header;
			try
			{
				FileDeserializer in (fromPath.c_str ());
				header.reset (new ScriptSubHeader (in));
			}
			catch (...) 
			{
				Win::ClearError ();
				header.reset ();
			}
			ScriptFileName fileName (transHdr->GetScriptId (), "local user", transHdr->GetProjectName ());
			char const * to = 0;
			if (header->GetPartCount () == 1)
			{
				to = toPath.GetFilePath (fileName.Get ());
			}
			else
			{
				to = toPath.GetFilePath (fileName.Get (header->GetPartNumber (), header->GetPartCount ()));
			}
			File::Copy (fromPath.c_str (), to);
			return true;
		}
		return false;
	}
	else if (ext.IsEqual ("snc") || ext.IsEqual ("cnk"))
	{
		char const * to = toPath.GetFilePath (fileName.c_str ());
		File::Copy (fromPath.c_str (), to);
		return true;
	}
	else if (ext.IsEqual ("znc"))
	{
		if (ateExtension)
		{
			// first three bits are version number
			MemFileReadOnly attach (fromPath.c_str ());
			if (!attach.GetSize ().IsLarge () && attach.GetSize ().Low () < 4)
				return true;
			unsigned char b = attach.GetBuf () [0];
			if ((b >> (ByteBits - VersionBits)) != Version)
				return true;
		}
		// Decompress scripts
		Decompress (fromPath, toPath, title);
		return true;
	}
	else if (!ext.IsEqual (_procConfig.GetPostproExt ()))
	{
		// prepare message displayed in dialog
		std::string info = "No user-defined processor defined for ";
		if (extension.empty ())
		{
			info += "an empty extension.";
		}
		else
		{
			info += "the extension ";
			info += fileExt;
		}
		TheAlertMan.PostInfoAlert (info);
		return false;
	}

	// User defined post processing
	FilePath tmpPath (fromFolder.c_str ());
	// Cleanup old scripts if any
	ShellMan::QuietDelete (0, tmpPath.GetFilePath ("*.snc"));
	ShellMan::QuietDelete (0, tmpPath.GetFilePath ("*.cnk"));
	// Cleanup old compressed script file if any
	ShellMan::QuietDelete (0, tmpPath.GetFilePath ("*.znc"));
	if (!File::Exists (fromPath.c_str ()))
		throw Win::Exception ("Source file for user preprocessing is absent", fromPath.c_str ());

	// build command line:
	// "executable name" "attachment name"
	std::string cmdLine = _procConfig.GetPostproCommand ();
	cmdLine += " \"";
	cmdLine += fromPath;
	cmdLine += "\"";

	RunUserTool (fromFolder.c_str (), cmdLine);

	std::string compressedFilePath (tmpPath.GetFilePath (_compressedFileName));
	if (File::Exists (compressedFilePath.c_str ()))
	{
		// User defined post processing resulted in the compressed script file
		Decompress (compressedFilePath, toPath, title);
	}
	else
	{
		// User defined post processing resulted in the uncompressed script files
		std::string allScripts (tmpPath.GetFilePath ("*.snc"));
		allScripts += '\0';
		allScripts += tmpPath.GetFilePath ("*.cnk");
		allScripts += '\0';
		allScripts += '\0';
		ShellMan::CopyFiles (0, allScripts.c_str (), toPath.GetDir ());
		ShellMan::QuietDelete (0, allScripts.c_str ());
	}
	return true;
}

std::string ScriptProcessor::Pack (SafePaths & scriptPaths, 
								   std::string const & scriptPath,
								   FilePath const & tmpFolder, 
								   std::string & extension,
								   std::string const & projectName) const
{
	std::string packedFilePath (tmpFolder.GetFilePath (_compressedFileName));
	// Compress scripts
	CompressOne (scriptPath, packedFilePath);
	std::string ext (_compressedFileExt);
	ext += '.';
	extension.insert (0, ext); // e.g. znc.cnk
	if (!_procConfig.NeedsUserPreprocessing ())
	{
		scriptPaths.Remember (packedFilePath);
		return packedFilePath;
	}

	// User defined preprocessing
	// Delete the old user result file
	File::DeleteNoEx (tmpFolder.GetFilePath (_procConfig.GetPreproResult ()));
	if (!File::Exists (packedFilePath.c_str ()))
		throw Win::Exception ("Source file for user preprocessing is absent", packedFilePath.c_str ());

	// build command line:
	// "executable name" "output file name" "script1" "script2" ...
	std::string cmdLine = _procConfig.GetPreproCommand ();

	cmdLine += " \"";
	cmdLine += _procConfig.GetPreproResult ();
	cmdLine += "\" \"";
	cmdLine += packedFilePath;
	cmdLine += "\"";
	if (_procConfig.PreproNeedsProjName ())
	{
		cmdLine += " \"";
		cmdLine += projectName;
		cmdLine += "\"";
	}

	RunUserTool (tmpFolder.GetDir (), cmdLine);
	
	std::string resultPath (tmpFolder.GetFilePath (_procConfig.GetPreproResult ()));

	if (File::Exists (resultPath.c_str ()))
	{
		scriptPaths.Remember (resultPath);
		PathSplitter splitter (resultPath);
		ext.assign (splitter.GetExtension ());
		if (ext [0] == '.')
			ext = ext.substr (1);
		ext += '.';
		extension.insert (0, ext); // e.g. zip.znc.cnk
	}
	else
	{
		if (_procConfig.CanSendUnprocessed ())
		{
			resultPath = packedFilePath;
		}
		else
			throw Win::Exception ("User-defined script preprocessor did not produce a file", resultPath.c_str ());
	}

	return resultPath;
}

void ScriptProcessor::Compress (SafePaths const & scriptPaths, std::string const & packedFilePath) const
{
	// Delete the old compressed result file
	File::DeleteNoEx (packedFilePath.c_str ());
	// Open files to compress and remember their names
	auto_vector<MemFileReadOnly> filesToCompress;
	std::vector<std::string> fileNames;
	for (SafePaths::iterator iter = scriptPaths.begin (); iter != scriptPaths.end (); ++iter)
	{
		char const * path = iter->c_str ();
		Assert (!File::IsFolder (path));
		std::unique_ptr<MemFileReadOnly> curFile (new MemFileReadOnly (path));
		filesToCompress.push_back (std::move(curFile));
		PathSplitter splitter (path);
		std::string name (splitter.GetFileName ());
		name += splitter.GetExtension ();
		fileNames.push_back (name);
	}
	// Create named block for each file to compress
	std::vector<InNamedBlock> blocks;
	for (unsigned int i = 0; i < filesToCompress.size (); ++i)
	{
		char const * buf = filesToCompress [i]->GetBuf ();
		File::Size size = filesToCompress [i]->GetSize ();
		InNamedBlock curBlock (fileNames [i], buf, size);
		blocks.push_back (curBlock);
	}
	// Compress all named blocks
	Compressor compressor (blocks);
	compressor.Compress ();
	// Write compressed file
	MemFileNew out (packedFilePath.c_str (), File::Size (compressor.GetPackedSize (), 0));
	std::copy (compressor.begin (), compressor.end (), out.GetBuf ());
}

void ScriptProcessor::CompressOne (std::string const & srcPath, std::string const & packedFilePath) const
{
	// Delete the old compressed result file
	File::DeleteNoEx (packedFilePath.c_str ());
	// Open file to compress and remember its name
	PathSplitter splitter (srcPath);
	std::string fileName (splitter.GetFileName ());
	fileName += splitter.GetExtension ();
	Assert (!File::IsFolder (srcPath.c_str ()));
	MemFileReadOnly fileToCompress (srcPath);
	// Create a named block
	char const * buf = fileToCompress.GetBuf ();
	File::Size size = fileToCompress.GetSize ();
	InNamedBlock block (fileName, buf, size);
	// Compress named block
	std::vector<InNamedBlock> blocks;
	blocks.push_back (block);
	Compressor compressor (blocks);
	compressor.Compress ();
	// Write compressed file
	MemFileNew out (packedFilePath.c_str (), File::Size (compressor.GetPackedSize (), 0));
	std::copy (compressor.begin (), compressor.end (), out.GetBuf ());
}

void ScriptProcessor::Decompress (std::string const & packedFilePath, FilePath const & toPath, std::string const & title) const
{
	TmpPath tmpPath;
	std::unique_ptr<OutNamedBlock> firstBlock (new FileOutBlock (tmpPath));
	OutBlockVector unpackedFiles (firstBlock);

	{
		MemFileReadOnly packedFile (packedFilePath.c_str ());
		Decompressor decompressor;
		unsigned char const * buf = reinterpret_cast<unsigned char const *>(packedFile.GetBuf ());
		File::Size size = packedFile.GetSize ();
		if (!decompressor.Decompress (buf, size, unpackedFiles))
			throw Win::Exception ("Corrupted compressed script file -- cannot unpack scripts.", title.c_str ());
	}

	for (unsigned int i = 0; i < unpackedFiles.size (); ++i)
	{
		OutNamedBlock & curBlock = unpackedFiles [i];
		curBlock.SaveTo (toPath);
	}
}

void ScriptProcessor::RunUserTool (char const * curFolder, std::string const & cmdLine) const
{
	Win::ChildProcess userTool (cmdLine);
	userTool.SetCurrentFolder (curFolder);
	userTool.ShowMinimizedNotActive ();
	if (userTool.Create (0))
	{
		// Wait for a tool to complete its task -- 50 sec.
		if (userTool.WaitForDeath (50000))
		{
			unsigned long exitCode = userTool.GetExitCode ();
			if (exitCode == 0)
				return;	// Tool completed its task successfuly

			std::string info = "The execution of your script processing program has failed."
								"\n\nThe program exited with the code: ";
			info += ToString (exitCode);
			info += '.';
			TheAlertMan.PostInfoAlert (info);
		}
		else
		{
			// Time-out
			std::string info = "The user-defined program is taking too long to finish."
							   "\nTerminating the user-defined program...";
			TheAlertMan.PostInfoAlert (info);
			userTool.Terminate ();
		}
	}
	LastSysErr lastError;
	if (lastError.IsFileNotFound () || lastError.IsPathNotFound ())
	{
		std::string info;
		info = "User command:\n\n\"";
		info += cmdLine;
		info += "\"\n\ncannot be executed because the system couldn't find"
			    "\nthe program or path specified.";
		TheAlertMan.PostInfoAlert (info);
		throw Win::Exception ();
	}
	throw Win::Exception ("Cannot process scripts");
}

bool ScriptProcessor::HasIgnoredExtension (std::string const & path)
{
	PathSplitter splitter (path.c_str ());
	Extension ext (splitter.GetExtension ());
	return IsIgnoredExtension (ext);
}

bool ScriptProcessor::IsIgnoredExtension (Extension const & ext)
{
	return ext.IsEqual ("gif") ||
		   ext.IsEqual ("vcf") ||
		   ext.IsEqual ("txt") ||
		   ext.IsEqual ("jpg");
}

ScriptProcessor::Extension::Extension (std::string const & extension) 
{
	// Filename extension is optional; 
	// If present it might include the leading period (.)
	if (!extension.empty ())
	{
		Assert (extension.size () < _MAX_EXT);
		if (extension [0] == '.')
			_ext = extension.substr (1);
		else
			_ext = extension;
	}
}

bool ScriptProcessor::Extension::IsEqual (char const * ext) const
{
	return IsFileNameEqual (_ext, ext);	 
}
