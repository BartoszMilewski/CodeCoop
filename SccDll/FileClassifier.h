#if !defined (FILECLASSIFIER_H)
#define FILECLASSIFIER_H
//------------------------------------
//  (c) Reliable Software, 2000 - 2006
//------------------------------------

#include <auto_vector.h>

class IDEContext;
class Catalog;
class StatusSequencer;

// Split IDE file list between Code Co-op projects

class FileListClassifier
{
public:
	FileListClassifier (int fileCount, char const **paths, IDEContext const * ideContext);
	FileListClassifier (int projId, char const **paths);

	class ProjectFiles
	{
	private:
		typedef std::set<int>::const_iterator FileIter;

	public:
		ProjectFiles (char const ** idePaths, int id, std::string const & name, std::string const & root)
			: _idePaths (idePaths),
			  _id (id),
			  _name (name),
			  _root (root)
		{}

		void AddFile (int fileIdx) { _files.insert (fileIdx); }
		int GetProjectId () const { return _id; }
		std::string const & GetProjectName () const { return _name; }
		std::string const & GetProjectRootPath () const { return _root; }
		std::string GetFileList () const;
		unsigned int GetFileCount () const { return _files.size (); }

		void FillStatus (long ideStatus [], StatusSequencer & statusSeq) const;

		class Sequencer
		{
		public:
			Sequencer (ProjectFiles const & files)
				: _idePaths (files._idePaths),
				  _cur (files.begin ()),
				  _end (files.end ())
			{}
			void Advance () { ++_cur; }
			bool AtEnd () const { return _cur == _end; }

			char const * GetFilePath () const { return _idePaths [*_cur]; }
			int GetIdeIndex () const { return *_cur; }
		private:
			char const **	_idePaths;
			FileIter		_cur;
			FileIter		_end;
		};

		friend class ProjectFiles::Sequencer;
	
	private:
		FileIter begin () const { return _files.begin (); }
		FileIter end () const { return _files.end (); }

	private:
		char const **	_idePaths;
		int				_id;
		std::string		_name;
		std::string		_root;
		// Set of indicies to _idePaths.
		// Each indicated file belongs to this Co-op project
		std::set<int>	_files;
	};

	typedef auto_vector<ProjectFiles>::const_iterator Iterator;
	Iterator begin () const { return _fileCatalog.begin (); }
	Iterator end () const { return _fileCatalog.end (); }

	bool IsEmpty () const { return _fileCatalog.size () == 0; }

private:
	void CatalogFiles (int ideFileCount, char const **paths, Catalog & catalog);

private:
	auto_vector<ProjectFiles>	_fileCatalog;
};

#endif
