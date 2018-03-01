#if !defined (SHARE_EX)
#define SHARE_EX
//---------------------------
// (c) Reliable Software 2000
//---------------------------

namespace Net
{
	class ShareException
	{
	public:
		ShareException (int errCode, char const * msg, char const * name, char const * path)
			: _errCode (errCode), _msg (msg)
		{
			if (name && name[0] != '\0')
				strncpy(_name, name, 127);
			else
				_name[0] = '\0';

			if (path && path[0] != '\0')
				strncpy(_path, path, 255);
			else
				_path[0] = '\0';
		}

		int GetError () const { return _errCode; }
		char const * GetMessage () const { return _msg; }
		char const * GetReason () const;
		char const * GetName() const { return _name; }
		char const * GetPath() const { return _path; }

	private:
		int			 _errCode;
		char const * _msg;
		char _name[128];
		char _path[256];
	};
}
#endif
