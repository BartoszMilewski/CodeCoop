//---------------------------------------------
// (c) Reliable Software 2001 -- 2003
//---------------------------------------------

#include "precompiled.h"
#include "Model.h"
#include "PathSequencer.h"
#include "ProgressMeter.h"
#include "OutputSink.h"
#include "NamedBlock.h"
#include "Compressor.h"
#include "Decompressor.h"
#include "Serialize.h"

#include <auto_vector.h>
#include <LightString.h>
#include <File/File.h>
#include <File/MemFile.h>
#include <Ex/Winex.h>

#include <memory>

class StopWatch
{
public:
	StopWatch ()
		: _frequency (0),
		  _startCount (0),
		  _stopCount (0)
	{
		::QueryPerformanceFrequency (reinterpret_cast<LARGE_INTEGER *>(&_frequency));
	}

	void Start ()
	{
		if (_frequency != 0)
			::QueryPerformanceCounter (reinterpret_cast<LARGE_INTEGER *>(&_startCount));
	}
	void Stop ()
	{
		if (_frequency != 0)
			::QueryPerformanceCounter (reinterpret_cast<LARGE_INTEGER *>(&_stopCount));
	}
	double GetElapsedTime () const
	{
		if (_frequency != 0)
		{
			__int64 countDiff = 0;
			if (_startCount < _stopCount)
			{
				countDiff = _stopCount - _startCount;
			}
			else if (_startCount > _stopCount)
			{
				countDiff = (0x7fffffffffffffff - _startCount) + _stopCount;
			}
			double diff = static_cast<double>(countDiff);
			double freq = static_cast<double>(_frequency);
			return diff / freq;
		}
		return 0.0;
	}

private:
	__int64	_frequency;
	__int64	_startCount;
	__int64	_stopCount;
};

//
// Model
//

Model::Model ()
{
}

void Model::Compress (FolderContents & files, unsigned int fileCount, std::string & packedFilePath, ProgressMeter & progress)
{
	try
	{
		progress.SetRange (0, fileCount + 2, 1);
		auto_vector<MemFileReadOnly> filesToCompress;
		std::vector<std::string> fileNames;
		std::vector<std::string> extensions;
		files.GetExtensionList (extensions);
		for (std::vector<std::string>::const_iterator extIter = extensions.begin ();
			 extIter != extensions.end ();
			 ++extIter)
		{
			FolderContents::FileSeq fileSeq = files.GetFileList (*extIter);
			for ( ; !fileSeq.AtEnd (); fileSeq.Advance ())
			{
				if (fileSeq.IsSelected ())
				{
					std::string const & path = fileSeq.GetPath ();
					progress.SetActivity (path.c_str ());
					progress.StepAndCheck ();
					Assert (!File::IsFolder (path.c_str ()));
					std::auto_ptr<MemFileReadOnly> curFile (new MemFileReadOnly (path.c_str ()));
					filesToCompress.push_back (curFile);
					PathSplitter splitter (path);
					std::string name (splitter.GetFileName ());
					name += splitter.GetExtension ();
					fileNames.push_back (name);
				}
			}
		}

		std::vector<InNamedBlock> blocks;
		unsigned long uncompressedSize = 0;
		for (unsigned int i = 0; i < filesToCompress.size (); ++i)
		{

			if (progress.WasCanceled ())
			{
				progress.Close ();
				return;
			}
			char const * buf = filesToCompress [i]->GetBuf ();
			File::Size size = filesToCompress [i]->GetSize ();
			uncompressedSize += size.Low ();
			InNamedBlock curBlock (fileNames [i], buf, size);
			blocks.push_back (curBlock);
		}
		progress.SetActivity ("Compressing files.");
		Compressor compressor (blocks);
		StopWatch timer;
		timer.Start ();
		compressor.Compress ();
		timer.Stop ();
		progress.Close ();
		TmpPath tmpPath;
		packedFilePath.assign (tmpPath.GetFilePath ("Test.znc"));
		// Write compressed file
		MemFileNew out (packedFilePath.c_str (), File::Size (compressor.GetPackedSize (), 0));
		std::copy (compressor.begin (), compressor.end (), out.GetBuf ());
		unsigned long compressedSize = compressor.GetPackedSize ();
		Msg info;
		info << "Uncompressed size = " << uncompressedSize << "; compressed size = " << compressedSize;
		double sizeAfter = compressedSize;
		double sizeBefore = uncompressedSize > 0 ? uncompressedSize : 1;
		double ratio = (1.0 - (sizeAfter / sizeBefore)) * 100.0;
		info << "\n\nCompression ratio: " << ratio << "%";
		info << "\n\nThe compression of " << filesToCompress.size () << " file(s) took " << timer.GetElapsedTime () << " second(s).";
		TheOutput.Display (info.c_str ());
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch ( ... )
	{
		TheOutput.Display ("Unknown exception during file compression.");
	}
	progress.Close ();
}

void Model::VerifyCompresion (FolderContents & files, unsigned int fileCount, std::string & packedFilePath, ProgressMeter & progress)
{
	try
	{
		progress.SetRange (0, fileCount + 1, 1);
		progress.SetActivity ("Decompressing files.");
		Decompressor decompressor;
		TmpPath tmpPath;
		std::auto_ptr<OutNamedBlock> firstBlock (new FileOutBlock (tmpPath));
		OutBlockVector unpackedFiles (firstBlock);
		MemFileReadOnly packedFile (packedFilePath.c_str ());
		StopWatch timer;
		timer.Start ();
		bool success = decompressor.Decompress (reinterpret_cast<unsigned char const *>(packedFile.GetBuf ()),
												packedFile.GetSize (), unpackedFiles);
		timer.Stop ();
		progress.SetCaption ("Comparing files.");
		unsigned int k = 0;
		std::vector<std::string> extensions;
		files.GetExtensionList (extensions);
		for (std::vector<std::string>::const_iterator extIter = extensions.begin ();
			 extIter != extensions.end ();
			 ++extIter)
		{
			FolderContents::FileSeq fileSeq = files.GetFileList (*extIter);
			for ( ; !fileSeq.AtEnd (); fileSeq.Advance ())
			{
				if (fileSeq.IsSelected ())
				{
					std::string const & path = fileSeq.GetPath ();
					Assert (k < unpackedFiles.size ());
					progress.StepAndCheck ();
					progress.SetActivity (path.c_str ());
					MemFileReadOnly curFile (path.c_str ());
					std::string verificationResult = unpackedFiles [k].Verify (curFile);
					if (!verificationResult.empty ())
					{
						Msg info;
						info << verificationResult;
						info << path;
						TheOutput.Display (info.c_str ());
					}
					++k;
				}
			}
		}
		progress.Close ();
		if (!success)
			TheOutput.Display ("Cyclic Redundancy Check failed");
		Msg info;
		info << "\n\nThe decompression of " << fileCount << " file(s) took " << timer.GetElapsedTime () << " second(s).";
		TheOutput.Display (info.c_str ());
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch ( ... )
	{
		TheOutput.Display ("Unknown exception during file decompression.");
	}
	progress.Close ();
}

