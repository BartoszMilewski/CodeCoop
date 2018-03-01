#if !defined (MONITOR_H)
#define MONITOR_H
//---------------------------------------------
// (c) Reliable Software 2005
//---------------------------------------------

#include <Win/Win.h>

namespace Dbg
{
	class Monitor
	{
	public:
		Monitor ();
		Monitor (std::string const & program);
		~Monitor ()
		{
			Detach ();
		}
		void Attach (std::string const & program);
		void Detach ();
		void Write (std::string const & msg) const;

		bool IsRunning () const { return !_monitorWin.IsNull (); }
		std::string const & GetSourceId () const { return _sourceId; }

		static const char CLASS_NAME [];
		static const char UM_DBG_MON_START [];
		static const char UM_DBG_MON_STOP [];
		// Display line in the debug output monitor
		// wParam = line length; lParam = pointer to the line
		static const char UM_PUT_LINE [];

	public:
		class Msg
		{
		public:
			Msg (std::string const & str);

			void AddInfo (std::string const & info) { _msg += info; }

			unsigned length () const { return _msg.length (); }
			char const * Get () const { return _msg.c_str (); }
			std::string GetThreadId () const;
			bool IsCurrentVersion () const { return _msg [0] == '1' || _msg [0] == '2'; }
			char const * c_str () const
			{
				if (length () > 2 && IsCurrentVersion ())
					return &_msg [2];
				return 0;
			}

			bool IsHeader () const
			{
				if (length () >= 2 && IsCurrentVersion ())
					return _msg [1] == 'H';
				return false;
			}

			bool IsBogus () const
			{
				return length () <= 2;
			}

		protected:
			Msg (std::string const & sourceId, char type);

		private:
			std::string	_msg;
		};

		class AttachMsg : public Msg
		{
		public:
			AttachMsg (std::string const & program)
				: Msg (program, 'H')
			{
				AddInfo (">>> START");
			}
		};

		class DetachMsg : public Msg
		{
		public:
			DetachMsg (std::string const & program)
				: Msg (program, 'H')
			{
				AddInfo ("<<< STOP");
			}
		};

		class InfoMsg : public Msg
		{
		public:
			InfoMsg (std::string const & sourceId, std::string const & info)
				: Msg (sourceId, 'I')
			{
				AddInfo (info);
			}
		};

		void Write (Dbg::Monitor::Msg const & msg) const;

	private:
		void FindMonitorWindow ();
		void Send (Dbg::Monitor::Msg const & msg) const;

	private:
		std::string			_sourceId;
		Win::Dow::Handle	_monitorWin;
	};
}

#endif
