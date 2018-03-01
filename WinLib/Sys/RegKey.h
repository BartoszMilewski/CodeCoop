#if !defined REGKEY_H
#define REGKEY_H
//------------------------------------
//  (c) Reliable Software, 1996 - 2008
//------------------------------------

#include <Win/Handles.h>

namespace Win
{
    class Placement;

#ifdef BORLAND_COMPILER
    template<>
    void Disposal<HKEY>::Dispose (RegKey::Handle h) throw ();
#endif
}

class MultiString;

namespace RegKey
{
	class Handle : public Win::Handle<HKEY>
	{
	public:
		Handle (HKEY h = 0) 
			: Win::Handle<HKEY> (h)
		{}

    	bool SetValueString (std::string const & valueName,
							 std::string const & value,
							 bool quiet = false)
		{
			return SetString (valueName, value, REG_SZ, quiet);
		}
		bool SetValueExpandedString (std::string const & valueName,
									 std::string const & value,
									 bool quiet = false)
		{
			return SetString (valueName, value, REG_EXPAND_SZ, quiet);
		}
		bool SetValueMultiString (std::string const & valueName,
								  MultiString const & value,
								  bool quiet = false);
		bool SetStringEncrypt (std::string const & valueName,
							   std::string const & value,
							   bool quiet = false);

    	bool SetValueLong (std::string const & valueName,
						   unsigned long value,
						   bool quiet = false);
    	bool SetValueBinary (std::string const & valueName,
							 void const *valueBuffer,
							 unsigned bufferSize,
							 bool quiet = false);

		std::string GetStringVal (std::string const & valueName) const;
		void GetMultiString (std::string const & valueName, MultiString & mString) const;
		bool GetStringDecrypt (std::string const & valueName, std::string & value) const;
	    bool GetValueLong (std::string const & valueName, unsigned long & value) const;

		//	GetValueBinary returns false with bytesNeeded = 0 for value not found,
		//						   false with bytesNeeded = n for buffer too small (n actual size needed)
		//						   true with bytesNeeded = n for value found (n actual bytes retrieved)
		bool GetValueBinary (std::string const & valueName,
							 void * valueBuffer,
							 unsigned long bufferSize,
							 unsigned long & bytesNeeded) const;

		bool IsKeyEmpty () const;
		bool IsValuePresent (std::string const & valueName) const;

	    void DeleteValue (std::string const & valueName) const;
	    void DeleteSubKey (std::string const & subKeyName) const;

	private:
		bool SetString (std::string const & valueName,
						std::string const & value,
						unsigned long strType,
						bool quiet);
		void GetString (std::string const & valueName, std::vector<unsigned char> & buf) const;
	};
	
	// Global functions
	void DeleteTree (Handle key);
	// Copy disjoin trees - no nested registry trees
	void CopyTree (Handle from, Handle to);  //  only handles value sizes up to MAX_PATH
	// Window Placement
	void SaveWinPlacement (Win::Placement const & placement, Handle regKey);
	void ReadWinPlacement (Win::Placement & placement, Handle regKey);

	class AutoHandle : public Win::AutoHandle<RegKey::Handle>
	{
	public:
		AutoHandle (HKEY h = 0) 
			: Win::AutoHandle<RegKey::Handle> (h)
		{}
	};

	class Root : public Handle
	{
	public:
		Root (HKEY key)
			: Handle (key)
		{
			// root keys are always in ThePathMap
		}
	};

	class LocalMachine : public Root
	{
	public:
		LocalMachine ()
			: Root (HKEY_LOCAL_MACHINE)
		{}
	};

	class CurrentUser : public Root
	{
	public:
		CurrentUser ()
			: Root (HKEY_CURRENT_USER)
		{}
	};

	class Existing : public AutoHandle
	{
	public:
	    Existing (Handle keyParent, std::string const & subKey);
	};

	class ReadOnly : public AutoHandle
	{
	public:
	    ReadOnly (Handle keyParent, std::string const & subKey);
	};

	class New : public AutoHandle
	{
	public:
	    New (Handle keyParent, std::string const & subKey);
	};

	class Check: public AutoHandle
	{
	public:
	    Check (Handle keyParent, std::string const & subKey, bool writeable = false);
	    bool Exists () const { return _exists; }
	private:
	    bool _exists;
	};

	class CheckWriteable: public Check
	{
	public:
		CheckWriteable (Handle keyParent, std::string const & subKey)
			: Check (keyParent, subKey, true)	// Check writeable
		{}
	};

	// HKEY_LOCAL_MACHINE
	//      Software
	//          Microsoft
	//              Windows
	//					CurrentVersion
	class System
	{
	public:
	    System ()
	       :_keyRoot (HKEY_LOCAL_MACHINE),
	        _keyMain (_keyRoot, "Software"),
	        _keyMicrosoft (_keyMain, "Microsoft")
	    {}

		std::string GetProgramFilesPath ();
		std::string GetRegisteredOwner ();

	private:
		Root		_keyRoot;
	    ReadOnly	_keyMain;
	    ReadOnly	_keyMicrosoft;
	};

	class ValueSeq
	{
	public:
	    ValueSeq (RegKey::Handle key);

	    void Advance ();
	    bool AtEnd () const { return _status == ERROR_NO_MORE_ITEMS; }
	    void Rewind () { _cur = -1; }

		std::string GetName () const;
		std::string GetString () const;
		void GetMultiString (MultiString & mString) const;
	    unsigned char const * GetBytes () const
		{
			Assert (_type == REG_BINARY);
			return &_value [0];
		}
	    unsigned long GetLong () const
	    {
			Assert (_type == REG_DWORD);
	        unsigned long const * p = 
				reinterpret_cast<unsigned long const *> (&_value [0]);
	        return *p;
	    }
	    unsigned long GetValueLen () const { return _valueLen; }
	    unsigned long GetType () const { return _type; }
		bool IsString () const { return _type == REG_SZ; }
		bool IsLong () const { return _type == REG_DWORD; }
		bool IsBinary () const { return _type == REG_BINARY; }
		bool IsMultiString () const { return _type == REG_MULTI_SZ; }
	    unsigned long Count () const { return _count; }

	private:
	    HKEY			_key;
	    unsigned long	_count;
	    unsigned long	_cur;
	    long			_status;
	    unsigned long	_type;
	    unsigned long	_nameLen;
		std::string		_name;
	    unsigned long	_valueLen;
		std::vector<unsigned char>	_value;
	};

	class Seq
	{
	public:
	    Seq (RegKey::Handle key);

	    void Advance ();
	    bool AtEnd () const { return _status == ERROR_NO_MORE_ITEMS; }
	    void Rewind () 
		{ 
			_cur = -1; 
			Advance ();
		}

	    std::string GetKeyName () const 
		{
			std::string name (&_name [0], _nameLen);
			return name; 
		}
	    std::string GetClassName () const 
		{ 
			std::string classStr (&_class [0], _classLen);
			return classStr; 
		}
	    FILETIME const & GetModificationTime () const { return _lastTime; }
	    unsigned long const  Count () const { return _count; }

	private:
	    HKEY     			_key;
		unsigned long		_count;
	    unsigned long		_cur;
	    long				_status;
	    unsigned long 		_nameLen;
		std::vector<char>	_name;
	    unsigned long 		_classLen;
		std::vector<char>	_class;
	    FILETIME			_lastTime;
	};
}

#endif
