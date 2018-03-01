//----------------------------------
// (c) Reliable Software 2005 - 2008
//----------------------------------

#include "precompiled.h"
#include "DistributorScriptSend.h"
#include "DispatcherCmd.h"
#include "DispatcherScript.h"
#include "Address.h"
#include "NamedBlock.h"
#include "Compressor.h"
#include "Email.h"
#include "EmailMessage.h"

#include <File/MemFile.h>
#include <File/SafePaths.h>
#include <Com/Shell.h>

void MailOrSaveDistributorBlock (std::string const & address,
								 std::string const & senderUserId, 
								 std::string const & senderHubId,
								 std::string const & licensee,
								 unsigned startNumber,
								 unsigned count,
								 std::string const & instructions,
								 bool isPreviewBeforeSending)
{
	std::unique_ptr<DistributorLicenseCmd> cmd (
		new DistributorLicenseCmd (licensee , startNumber, count));
	// Test correctness
	{
		unsigned char buf [2000];
		MemorySerializer out (buf, 2000);
		cmd->Serialize (out);
		MemoryDeserializer in (buf, 2000);
		DistributorLicenseCmd check;
		long type = in.GetLong ();
		check.Deserialize (in, 0);
		if (check.Licensee () != licensee
			|| check.StartNum () != startNumber
			|| check.Count () != count)
		{
			throw Win::Exception ("Bug: encoding and decoding of distribution license don't match");
		}
	}
	// Create and save script
	TmpPath outDir;
	std::string fileName;

	SaveDispatcherScript (std::move(cmd), 
		senderUserId, 
		DispatcherAtHubId, 
		senderHubId,
		outDir,
		fileName); // out parameter

	SafePaths safePaths;
	std::string scriptPath = outDir.GetFilePath (fileName);
	safePaths.Remember (scriptPath);
	std::string attachPath;
	if (address.empty ()) // don't email, save on desktop
	{
		FilePath desktopPath;
		ShellMan::DesktopFolder desktop;
		desktop.GetPath (desktopPath);
		attachPath = desktopPath.GetFilePath ("DistributorLicenses.znc");
	}
	else
	{
		attachPath = outDir.GetFilePath ("license.znc");
		safePaths.Remember (attachPath);
	}

	{
		MemFileReadOnly fileToCompress (scriptPath);
		// Create a named block
		char const * buf = fileToCompress.GetBuf ();
		File::Size size = fileToCompress.GetSize ();
		InNamedBlock block (fileName, buf, size);
		// Compress named block
		std::vector<InNamedBlock> blocks;
		blocks.push_back (block);
		Compressor compressor (blocks);
		compressor.Compress ();
		MemFileNew out (attachPath, File::Size (compressor.GetPackedSize (), 0));
		std::copy (compressor.begin (), compressor.end (), out.GetBuf ());
	}

	if (!address.empty ())
	{
		if (!TheEmail.IsValid ())
			TheEmail.Refresh ();
		Mailer mailer (TheEmail);
		OutgoingMessage mailMsg;

		mailMsg.SetSubject ("Code Co-op Distribution Licenses");
		mailMsg.AddFileAttachment (attachPath);
		mailMsg.SetText (instructions);
		if (isPreviewBeforeSending)
			mailer.Save (mailMsg, address);
		else
			mailer.Send (mailMsg, address);
	}
}


