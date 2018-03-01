#if !defined (CMDVEC_H)
#define CMDVEC_H
//----------------------------------
// (c) Reliable Software 1998 - 2005
//----------------------------------

#include <StringOp.h>
#include <Ctrl/Output.h>

namespace Win { class Accelerator; }

namespace Cmd
{
	enum Status
	{
		Disabled,		// Command not available
		Enabled,		// Command available
		Checked,		// Command available and check marked
		Invisible		// Command not displayed in menu
	};

	//-------------------
	// Vector of commands
	//-------------------

	class Vector
	{
	public:
		int Cmd2Id (char const * cmdName) const;
		virtual void Execute (int cmdId) const = 0;
		virtual Status Test (int cmdId) const = 0;
		virtual Status Test (char const * cmdName) const = 0;
		virtual ~Vector() {};
		
	protected:
		enum { cmdIDBase = 4000 };
		int GetIndex (char const * cmdName) const;
		int Id2Index (int id) const;

		typedef std::map<char const *, int, NocaseNameCmp> CmdMap;

		CmdMap					_cmdMap;
	};

	// An interface implemented by specific controllers.
	// An executor is usually used by a handler for
	// a control, such as a toolbar, to enable the execution of commands

	class Executor
	{
	public:
		virtual ~Executor () {}
		virtual bool IsEnabled (std::string const & cmdName) const = 0;
		virtual void ExecuteCommand (std::string const & cmdName) = 0;
		virtual void ExecuteCommand (int cmdId) = 0;
		virtual void PostCommand (std::string const & cmdName,
								  NamedValues const & namedArgs)
		{}
		virtual void DisableKeyboardAccelerators (Win::Accelerator * accel) = 0;
		virtual void EnableKeyboardAccelerators () = 0;
	};

	//-------------
	// Command item
	//-------------
	template <class T>
	class Item
	{
	public:
		char const * _name;			// The official name of the command
		void (T::*_exec)();			// Pointer to member--to execute the command
		Status (T::*_test)() const;	// PTM--to find the command status (maybe null->always on)
		char const * _help;			// help string
	};

	template <class T>
	class VectorExec: public Vector
	{
	public:
		VectorExec (Cmd::Item<T> const * cmdTable, T * commander);
		void Execute (int cmdId) const;
		Status Test (int cmdId) const;
		Status Test (char const * cmdName) const;
		char const * GetHelp (int cmdId) const;
		char const * GetName (int cmdId) const;
	protected:
		T *						_commander;
		Cmd::Item<T> const *	_cmd;
	};


	template <class T>
	VectorExec<T>::VectorExec (Cmd::Item<T> const * cmdTable, T * commander)
		: _cmd (cmdTable),
		  _commander (commander)
	{
		//Add command names to the map
		for (int j = 0; _cmd [j]._exec != 0; j++)
		{
			_cmdMap.insert (CmdMap::value_type (_cmd [j]._name, j));
		}
	}

	template <class T>
	void VectorExec<T>::Execute (int cmdId) const
	{
		Assert (cmdId != -1);
		Assert (_commander != 0);
		int idx = Id2Index (cmdId);
		Assert (_cmd [idx]._exec != 0);
		(_commander->*_cmd [idx]._exec)();
	}

	template <class T>
	Cmd::Status VectorExec<T>::Test (int cmdId) const throw ()
	{
		try
		{
			Assert (cmdId != -1);
			Assert (_commander != 0);
			int idx = Id2Index (cmdId);
			Assert (idx != -1);
			if (_cmd [idx]._test != 0)
				return (_commander->*_cmd [idx]._test)();
			else
				return Enabled; // always available
		}
#if defined (NDEBUG)
		catch ( ... )
		{
			return Cmd::Disabled;
		}
#else
		catch (Win::Exception e)
		{
			std::string info ("Exception during command testing -- cmd id = ");
			info += ToString (cmdId);
			Out::Sink::DisplayException (e, 0, "", info.c_str ());
			return Cmd::Disabled;
		}
		catch ( ... )
		{
			std::string info ("Exception during command testing -- cmd id = ");
			info += ToString (cmdId);
			Out::Sink output;
			output.Init ("Application", info.c_str ());
			output.Display ("Unknown exception");
			return Cmd::Disabled;
		}
#endif
	}

	template <class T>
	Cmd::Status VectorExec<T>::Test (char const * cmd) const throw ()
	{
		int cmdId = Cmd2Id (cmd);
		return Test (cmdId);
	}

	template <class T>
	char const * VectorExec<T>::GetHelp (int cmdId) const
	{
		Assert (cmdId != -1);
		int idx = Id2Index (cmdId);
		Assert (idx != -1);
		return _cmd [idx]._help;
	}

	template <class T>
	char const * VectorExec<T>::GetName (int cmdId) const
	{
		Assert (cmdId != -1);
		int idx = Id2Index (cmdId);
		Assert (idx != -1);
		return _cmd [idx]._name;
	}
}

#endif
