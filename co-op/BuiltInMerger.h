#if !defined (BUILTINMERGER_H)
#define BUILTINMERGER_H
//------------------------------------
//  (c) Reliable Software, 2006 - 2007
//------------------------------------

class PhysicalFile;
class Restorer;
namespace Progress { class Meter; }
class MergerArguments;

class BuiltInMerger
{
public:
	BuiltInMerger (PhysicalFile & file);
	BuiltInMerger (PhysicalFile & file, Restorer const & restorer);
	BuiltInMerger (MergerArguments const & args);

	void MergeFiles (Progress::Meter & meter);
	void MergeFiles (std::string const & mergeResultPath);

private:
	void DoMerge (std::vector<char> & mergeResultBuf, Progress::Meter & meter);

private:
	PhysicalFile *	_file;
	std::string		_projectVersion;
	std::string		_referenceVersion;
	std::string		_syncVersion;
};

#endif
