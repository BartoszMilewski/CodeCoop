#if !defined MODEL_H
#define MODEL_H
//---------------------------------------------------
//	(c) Reliable Software, 2001 -- 2003
//---------------------------------------------------

#include <string>

class ProgressMeter;
class FolderContents;

class Model
{
public:
	Model ();

	// Commands
	void Compress (FolderContents & files, unsigned int fileCount, std::string & packedFilePath, ProgressMeter & progress);
	void VerifyCompresion (FolderContents & files, unsigned int fileCount, std::string & packedFilePath, ProgressMeter & progress);

private:
};

#endif

