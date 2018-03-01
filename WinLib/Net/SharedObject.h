#if !defined (SHAREDOBJECT_H)
#define SHAREDOBJECT_H
//---------------------------
// (c) Reliable Software 2000-04
//---------------------------

namespace Net
{
	class SharedObject
	{
	public:
		enum EType 
		{ 
			DiskTree	= 0, 
			PrintQueue	= 1, 
			Device		= 2, 
			IPC			= 3 
		};

	public:
		SharedObject (std::string const & netname,		// net name of shared object, len < 50
					  std::string const & path,			// path
					  EType type)						// share type
			: _netname (netname), _path (path), _type (type)
		{
			// convert strings to wide strings
			_wnetname.assign (netname.begin (), netname.end ());
			_wpath.assign (path.begin (), path.end ());
		}

		void SetComment (std::string const & comment)
		{
			_comment = comment;
			_wcomment.assign (comment.begin (), comment.end ());
		}

		wchar_t * NetName () const { return const_cast<wchar_t *> (&_wnetname [0]); }
		wchar_t * Path () const { return const_cast<wchar_t *> (&_wpath [0]); }
		wchar_t * Comment () const { return const_cast<wchar_t *> (&_wcomment [0]); }

		char * GetNetName () const { return const_cast<char *> (&_netname [0]); }
		char * GetPath () const { return const_cast<char *> (&_path [0]); }
		char * GetComment () const { return const_cast<char *> (&_comment [0]); }

		unsigned long GetType () const { return static_cast<unsigned long> (_type); }

	private:
		// Non-unicode for compatibility with Win98
		std::string	_netname;		// netname of shared object
		std::string	_path;			// path of shared object
		std::string	_comment;		// remark

		std::wstring	_wnetname;		// netname of shared object
		std::wstring	_wpath;			// path of shared object
		std::wstring	_wcomment;		// remark

		EType			_type;
	};

	class SharedFolder : public SharedObject
	{
	public:
		SharedFolder (std::string const & netname, std::string const & path, std::string const & comment)
			: SharedObject (netname, path, SharedObject::DiskTree)
		{
			SetComment (comment);
		}
	};
}
#endif
