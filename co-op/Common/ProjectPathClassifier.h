#if !defined (PROJECTPATHCLASSIFIER_H)
#define PROJECTPATHCLASSIFIER_H
//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "ProjectData.h"

namespace Project
{
	class PathClassifier
	{
	public:
		PathClassifier (std::vector<Project::Data> const & projects)
			: _projects (projects)
		{
			for (unsigned idx = 0; idx < projects.size (); ++idx)
			{
				Project::Data const * project = &projects [idx];
				FullPathSeq pathSeq (project->GetRootDir ());
				std::string projectPathHead;
				if (pathSeq.HasDrive ())
				{
					projectPathHead = pathSeq.GetHead ();
				}
				else
				{
					Assume (pathSeq.IsUNC (), project->GetRootDir ());
					projectPathHead = pathSeq.GetServerName ();
				}
				_classifiedPaths.insert (std::make_pair(projectPathHead, project));
			}
		}

	private:
		typedef std::multimap<std::string, Project::Data const *>::const_iterator ClassifiedPathConstIter;

	public:
		class ProjectSequencer
		{
		public:
			ProjectSequencer (ClassifiedPathConstIter begin, ClassifiedPathConstIter end)
				: _cur (begin),
				  _end (end)
			{}

			bool AtEnd () const { return _cur == _end; }
			void Advance ()
			{
				std::string prevKey = _cur->first;
				++_cur;
				Assert (_cur == _end || _cur->first == prevKey);
			}

			int GetProjectId () const { return _cur->second->GetProjectId (); }
			char const * GetRootPath () const { return _cur->second->GetRootDir (); }
			std::string const & GetProjectName () const { return _cur->second->GetProjectName (); }

		private:
			ClassifiedPathConstIter	_cur;
			ClassifiedPathConstIter	_end;
		};

		class Sequencer
		{
		public:
			Sequencer (PathClassifier const & pathClassifier)
				: _classifiedPaths (pathClassifier._classifiedPaths)
			{
				Assert (!_classifiedPaths.empty ());
				ClassifiedPathConstIter firstPath = _classifiedPaths.begin ();
				std::pair<ClassifiedPathConstIter, ClassifiedPathConstIter> range =
					_classifiedPaths.equal_range (firstPath->first);
				_curRangeStart = range.first;
				_curRangeStop = range.second;
				Assert (_curRangeStart != _curRangeStop);
			}

			bool AtEnd () const { return _curRangeStart == _classifiedPaths.end (); }
			void Advance ()
			{
				if (_curRangeStop == _classifiedPaths.end ())
				{
					_curRangeStart = _classifiedPaths.end ();
				}
				else
				{
					std::pair<ClassifiedPathConstIter, ClassifiedPathConstIter> range =
						_classifiedPaths.equal_range (_curRangeStop->first);
					_curRangeStart = range.first;
					_curRangeStop = range.second;
				}
			}
			ProjectSequencer GetProjectSequencer () const
			{
				ProjectSequencer seq (_curRangeStart, _curRangeStop);
				return seq;
			}

		private:
			std::multimap<std::string, Project::Data const *> const &	_classifiedPaths;
			ClassifiedPathConstIter	_curRangeStart;
			ClassifiedPathConstIter	_curRangeStop;
		};

		friend class Sequencer;

	private:
		std::vector<Project::Data> const &					_projects;
		std::multimap<std::string, Project::Data const *>	_classifiedPaths;
	};
}

#endif
