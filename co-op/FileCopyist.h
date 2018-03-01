#if !defined (FILECOPYIST_H)
#define FILECOPYIST_H
//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include <auto_vector.h>
#include <StringOp.h>
#include <File/File.h>

namespace Ftp
{
	class Login;
}
namespace ShellMan
{
	class CopyRequest;
}
namespace Progress
{
	class Meter;
}
class FileCopyRequest;
class PartialPathSeq;

class FileCopyist
{
	class CopyTreeNode
	{
		friend class FileCopyist;
		typedef std::pair<std::string, std::string> CopyItem;

	public:
		CopyTreeNode (std::string const & folderName)
			: _folderName (folderName)
		{}

		void AddCopyItem (std::string const & srcFullPath, PartialPathSeq & tgtSeq);

		void BuildCopyList (File::Vpath & currentPath, ShellMan::CopyRequest & request) const;

		std::string const & GetFolderName () const { return _folderName; }

		bool IsEmpty () const { return _subFolders.size () == 0 && _files.size () == 0; }

	private:
		void SetFolderName (std::string const & folderName) { _folderName = folderName; }

	private:
		std::string					_folderName;
		auto_vector<CopyTreeNode>	_subFolders;
		std::vector<CopyItem>		_files;
	};

	class IsEqualFolderName : public std::unary_function<CopyTreeNode const *, bool>
	{
	public:
		IsEqualFolderName (char const * folderName)
			: _folderName (folderName)
		{}

		bool operator () (CopyTreeNode const * node)
		{
			return ::IsFileNameEqual (_folderName, node->GetFolderName ());
		}

	private:
		std::string	_folderName;
	};

public:
	FileCopyist (FileCopyRequest const & request);

	std::string const & GetTargetPath () const { return _root.GetFolderName (); }
	bool HasFilesToCopy () const { return !_root.IsEmpty (); }
	bool IsOverwriteExisting () const { return _overwrite; }

	void RememberFile (std::string const & srcFullPath, std::string const & tgtRelPath);
	void RememberSourceFolder (std::string const & folderRelPath)
	{
		_sourceFolders.push_back (folderRelPath);
	}
	bool DoCopy ();
	void DoUpload (Ftp::Login const & login,
				   std::string const & projectName,
				   std::string const & targetFolder);

private:
	bool			_overwrite;
	bool			_completeCleanupPossible;	// true when root folder can be deleted 
	CopyTreeNode	_root;
	std::vector<std::string>	_sourceFolders;
};

#endif
