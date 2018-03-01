#if !defined (COPYREQUEST_H)
#define COPYREQUEST_H
//------------------------------------
//  (c) Reliable Software, 1998 - 2008
//------------------------------------

#include <Com/Shell.h>

namespace ShellMan
{
	class FileRequest
	{
	public:
		FileRequest ();

		bool empty () const { return _from.empty (); }

		void MakeItQuiet ()
		{
			_flags |= FOF_SILENT;			// No UI
		}

		void AddFile (char const * path);
		void AddFile (std::string const & path);
		void MakeReadWrite ();
		void MakeReadOnly ();
		void DoDelete (Win::Dow::Handle win, bool quiet = false);

		class Sequencer
		{
		public:
			Sequencer (ShellMan::FileRequest const & request)
				: _paths (request._from),
				  _current (0),
				  _end (_paths.length ())
			{}

			bool AtEnd () const { return _current >= _end; }
			void Advance ()
			{
				_current = _paths.find ('\0', _current);
				if (_current == std::string::npos)
					_current = _paths.length ();
				else
					_current++;
			}

			char const * GetFilePath () const { return &_paths [_current]; }

		private:
			std::string const &	_paths;
			unsigned _current;
			unsigned _end;
		};

		friend class Sequencer;

	protected:
		FILEOP_FLAGS	_flags;
		std::string		_from;
	};

	class CopyRequest : public FileRequest
	{
	public:
		CopyRequest (bool quiet = false);

		void OverwriteExisting ()
		{
			_flags |= FOF_NOCONFIRMATION;	//Respond with Yes to All for any dialog box that is displayed.
		}
		void DoCopy (Win::Dow::Handle win, char const * title);
		void AddCopyRequest (char const * from, char const * to);
		void AddCopyRequest (std::string const & from, std::string const & to)
		{
			AddCopyRequest(from.c_str(), to.c_str());
		}
		void MakeDestinationReadWrite () const;
		void MakeDestinationReadOnly () const;
		void Cleanup () const;
#if !defined (NDEBUG)
		void Dump ();
#else
		void Dump () {}
#endif

	private:
		std::string			_to;
		std::vector<int>	_willOverwrite;
	};
}

#endif
