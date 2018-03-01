//----------------------------------
// (c) Reliable Software 1997 - 2008
//----------------------------------

#include <WinLibBase.h>
#include "RegKey.h"

#include <Win/Geom.h>
#include <Sys/SysVer.h>
#include <Sys/Crypto.h>
#include <Sys/Synchro.h>
#include <StringOp.h>

namespace RegKey
{
	// map handles into pairs: <parent handle, key name>
	class PathMap
	{
	public:
		PathMap ();
		~PathMap ();
		// Revisit: switch from using HKEY to RegKey::Handle
		void Add (HKEY h, HKEY keyParent, std::string const & subKeyName)
		{
			Win::Lock lock (_critSect);
			_map [h] = std::make_pair (keyParent, subKeyName);
		}
		std::string MakePath (HKEY keyParent, std::string const & subKeyName);
		std::string GetPath (HKEY h);
		void Remove (HKEY h) throw ();
	private:
		Win::CritSection	_critSect;
		std::map<HKEY, std::pair<HKEY, std::string> > _map;
	};

	PathMap ThePathMap;
}

template<>
void Win::Disposal<RegKey::Handle>::Dispose (RegKey::Handle h) throw ()
{
	::RegCloseKey (h.ToNative ());
	RegKey::ThePathMap.Remove (h.ToNative ());
}

namespace RegKey
{
	PathMap::PathMap ()
	{
		// add top-level keys
		_map [HKEY_LOCAL_MACHINE] = std::make_pair (HKEY (0), "HKEY_LOCAL_MACHINE");	
		_map [HKEY_CURRENT_USER] = std::make_pair (HKEY (0), "HKEY_CURRENT_USER");	
		_map [HKEY_USERS] = std::make_pair (HKEY (0), "HKEY_USERS");	
		_map [HKEY_CLASSES_ROOT] = std::make_pair (HKEY(0), "HKEY_CLASSES_ROOT");
	}

	PathMap::~PathMap ()
	{
#if !defined (NDEBUG)
		size_t left = _map.size ();
		if (left > 4)
		{
			std::string list;
			typedef std::map<HKEY, std::pair<HKEY, std::string> >::const_iterator Iter;
			for (Iter it = _map.begin (); it != _map.end (); ++it)
			{
				list += it->second.second;
				list += "\n";
			}
			::MessageBox (0, list.c_str (), "List of leaked registry keys", MB_OK | MB_ICONERROR);
		}
#endif
	}
		
	std::string PathMap::MakePath (HKEY keyParent, std::string const & subKeyName)
	{
		// GetPath takes a lock
		std::string path = GetPath (keyParent);
		path += '\\';
		path += subKeyName;
		return path;
	}

	std::string PathMap::GetPath (HKEY h)
	{
		Win::Lock lock (_critSect);
		std::string path;
		while (h != 0)
		{
			std::pair<HKEY, std::string> pair = _map [h];
			h = pair.first;
			path.insert (0, pair.second);
			path.insert (0, 1, '\\');
		}
		path.erase (path.begin ());
		return path;
	}

	void PathMap::Remove (HKEY h) throw ()
	{
		Win::Lock lock (_critSect);
		_map.erase (h);
	}

	bool Handle::SetValueMultiString (std::string const & valueName,
									  MultiString const & values,
									  bool quiet)
	{
		return SetString (valueName, values.str (), REG_MULTI_SZ, quiet);
	}

	bool Handle::SetString (std::string const & valueName,
							std::string const & value,
							unsigned long strType,
							bool quiet)
	{
		Assert (strType == REG_SZ || strType == REG_EXPAND_SZ || strType == REG_MULTI_SZ);
		long err = ::RegSetValueEx (H (), 
									valueName.c_str (), 
									0, 
									strType, 
									reinterpret_cast<unsigned char const *> (value.c_str ()),
									value.length () + 1);	// // Plus terminating '\0' character
		if (!quiet && err != ERROR_SUCCESS)
		{
			throw Win::Exception ("Registry operation failed. Writing a string.",
								  valueName.c_str (), err);
		}
		return err == ERROR_SUCCESS;
	}

	bool Handle::SetStringEncrypt (std::string const & valueName,
								   std::string const & value,
								   bool quiet)
	{
		Crypt::String encryptor (value, valueName);
		unsigned cryptPassSize;
		unsigned char const * cryptPass = encryptor.GetData (cryptPassSize);
		return SetValueBinary (valueName, cryptPass, cryptPassSize, quiet);
	}

	bool Handle::SetValueLong (std::string const & valueName, unsigned long value, bool quiet)
	{
		long err = ::RegSetValueEx (H (), 
									valueName.c_str (),
									0,
									REG_DWORD,
									reinterpret_cast<unsigned char const *> (&value),
									sizeof (unsigned long));
		if (!quiet && err != ERROR_SUCCESS)
		{
			throw Win::Exception ("Registry operation failed. Writing a long.",
								  valueName.c_str (), err);
		}
		return err == ERROR_SUCCESS;
	}

	bool Handle::SetValueBinary (std::string const & valueName,
								 void const * valueBuffer,
								 unsigned bufferSize,
								 bool quiet)
	{
		long err = ::RegSetValueEx (H (), 
									valueName.c_str (),
									0,
									REG_BINARY,
									(BYTE const *) valueBuffer,
									bufferSize);
		if (!quiet && err != ERROR_SUCCESS)
		{
			throw Win::Exception ("Registry operation failed. Writing a binary.",
								  valueName.c_str (), err);
		}
		return err == ERROR_SUCCESS;
	}

	std::string Handle::GetStringVal (std::string const & valueName) const
	{
		std::vector<unsigned char> buf;
		GetString (valueName, buf);
		if (buf.size () != 0)
		{
			// Remove terminating '\0' character
			buf.pop_back ();
		}
		return std::string (buf.begin (), buf.end ());
	}

	void Handle::GetMultiString (std::string const & valueName, MultiString & mString) const
	{
		std::vector<unsigned char> buf;
		GetString (valueName, buf);
		if (buf.size () > 1)
		{
			// Remove terminating '\0' character
			mString.Assign (reinterpret_cast<char const *>(&buf [0]), buf.size () - 1);
		}
	}

	void Handle::GetString (std::string const & valueName,
							std::vector<unsigned char> & buf) const
	{
		unsigned long bufSize = 0;
		unsigned long type;
		long err = ::RegQueryValueEx (H (),
									  valueName.c_str (),
									  0, // reserved
									  &type,
									  0, // no buffer, query size only
									  &bufSize);

		if (err == ERROR_FILE_NOT_FOUND) // Value not found
			return;
		if (err != ERROR_SUCCESS)
			throw Win::Exception ("Registry operation failed. Cannot query value.",
								  valueName.c_str (), err);
		if (type != REG_SZ && type != REG_EXPAND_SZ && type != REG_MULTI_SZ)
			throw Win::Exception ("Registry operation failed. Wrong type of value.",
								  valueName.c_str ());

		if (bufSize != 0)
		{
			buf.resize (bufSize);
			err = ::RegQueryValueEx (H (),
									 valueName.c_str (),
									 0,
									 &type,
									 &buf [0],
									 &bufSize);
			if (err != ERROR_SUCCESS)
				throw Win::Exception ("Registry operation failed. Cannot retrieve key string value.", 
									  valueName.c_str (), err);
		}
	}

	bool Handle::GetStringDecrypt (std::string const & valueName,
								   std::string & value) const
	{
		unsigned long binSize;
		try
		{
			GetValueBinary (valueName, 0, 0, binSize);
		}
		catch (...)
		{
			binSize = 0;
		}

		if (binSize == 0)
			return false;

		std::vector<unsigned char> binValue (binSize);
		GetValueBinary (valueName, &binValue [0], binValue.size (), binSize);
		Assert (binValue.size () == binSize);
		Crypt::String decrypt (&binValue [0], binSize);
		value = decrypt.GetPlainText ();
		return true;
	}

	bool Handle::GetValueLong (std::string const & valueName, unsigned long & value) const
	{
		unsigned long type = 0;
		unsigned long size = sizeof (unsigned long);
		unsigned long dw = 1;

		long err = ::RegQueryValueEx (H (),
									valueName.c_str (),
									0,
									&type,
									reinterpret_cast<unsigned char *> (&dw),
									&size);

		if (err == ERROR_FILE_NOT_FOUND)
		{
			// Value not found
			Win::ClearError ();
			return false;
		}
		if (type != REG_DWORD)
			throw Win::Exception ("Registry operation failed. Wrong type of value.",
								  valueName.c_str ());
		if (err != ERROR_SUCCESS)
			throw Win::Exception ("Registry operation failed. Cannot retrieve key binary value.",
								  valueName.c_str (), err);
		value = dw;
		return true;
	}

	bool Handle::GetValueBinary (std::string const & valueName,
								 void * valueBuffer,
								 unsigned long bufferSize,
								 unsigned long & bytesNeeded) const
	{
		unsigned long type = 0;
		unsigned long cb = bufferSize;

		long err = ::RegQueryValueEx (H (),
									  valueName.c_str (),
									  0,
									  &type,
									  (LPBYTE) valueBuffer,
									  &cb);

		if (err == ERROR_FILE_NOT_FOUND)
		{
			// Value not found
			Win::ClearError ();
			bytesNeeded = 0;
			return false;
		}

		if (type != REG_BINARY)
			throw Win::Exception ("Registry operation failed. Wrong type of value.",
								  valueName.c_str ());

		if (err == ERROR_MORE_DATA)
		{
			//	Buffer too small
			Win::ClearError ();
			bytesNeeded = cb;	//	how many bytes needed
			return false;
		}

		if (err != ERROR_SUCCESS)
			throw Win::Exception ("Registry operation failed. Cannot retrieve key binary value.",
								  valueName.c_str (), err);

		bytesNeeded = cb;	//	how bytes actually filled in
		return true;
	}

	void Handle::DeleteValue (std::string const & valueName) const
	{
		long err = ::RegDeleteValue (H (), valueName.c_str ());
		if (err != ERROR_SUCCESS)
		{
			if (err == ERROR_FILE_NOT_FOUND)
				Win::ClearError ();
			else
				throw Win::Exception ("Registry operation failed - cannot delete value",
									  valueName.c_str (), err);
		}
	}

	void Handle::DeleteSubKey (std::string const & subKeyName) const
	{
		long err = ::RegDeleteKey (H (), subKeyName.c_str ());
		if (err != ERROR_SUCCESS)
		{
			if (err == ERROR_FILE_NOT_FOUND)
				Win::ClearError ();
			else
				throw Win::Exception ("Registry operation failed - cannot delete key",
									  ThePathMap.MakePath (H (), subKeyName).c_str (), 
									  err);
		}
	}

	bool Handle::IsKeyEmpty () const
	{
		long    status;
		unsigned long	valueCount;
		unsigned long   subkeyCount;
		status = ::RegQueryInfoKey (H (),			// handle of key to query 
									0,				// address of buffer for class string 
									0,				// address of size of class string buffer 
									0,				// reserved 
									&subkeyCount,	// address of buffer for number of sub-key 
									0,				// address of buffer for longest sub-key name length  
									0,				// address of buffer for longest class string length 
									&valueCount,	// address of buffer for number of value entries 
									0,				// address of buffer for longest value name length 
									0,				// address of buffer for longest value data length 
									0,				// address of buffer for security descriptor length 
									0);				// address of buffer for last write time 
		if (status != ERROR_SUCCESS)
			throw Win::Exception ("Registry operation failed - cannot query key info", 
									ThePathMap.GetPath (H ()).c_str (), 
									status);
		return valueCount + subkeyCount == 0;
	}

	bool Handle::IsValuePresent (std::string const & valueName) const
	{
		for (RegKey::ValueSeq seq (*this); !seq.AtEnd (); seq.Advance ())
		{
			std::string name = seq.GetName ();
			if (name == valueName)
				return true;
		}
		return false;
	}

	void SaveWinPlacement (Win::Placement const & placement, Handle regKey)
	{
		regKey.SetValueLong ("Flags", placement.GetFlags (), true); // Quiet
		Win::Point pt;
		placement.GetMinCorner (pt);
		regKey.SetValueLong ("XMin", pt.x, true);
		regKey.SetValueLong ("YMin", pt.y, true);
		placement.GetMaxCorner (pt);
		regKey.SetValueLong ("XMax", pt.x, true);
		regKey.SetValueLong ("YMax", pt.y, true);
		regKey.SetValueLong ("Maximize", placement.IsMaximized () ? 1 : 0, true);
		regKey.SetValueLong ("Minimize", placement.IsMinimized () ? 1 : 0, true);
		Win::Rect rect;
		placement.GetRect (rect);
		regKey.SetValueLong ("Top", rect.Top (), true);
		regKey.SetValueLong ("Left", rect.Left (), true);
		regKey.SetValueLong ("Width", rect.Width (), true);
		regKey.SetValueLong ("Height", rect.Height (), true);
	}

	void ReadWinPlacement (Win::Placement & placement, RegKey::Handle regKey)
	{
		if (!regKey.IsNull ())
		{
			unsigned long value;
			if (regKey.GetValueLong ("Maximize", value))
			{
				if (value == 1)
					placement.SetMaximized ();
			}
			else
				return;
			if (regKey.GetValueLong ("Minimize", value))
			{
				if (value == 1)
					placement.SetMinimized ();
			}
			else
				return;
			// Normal window rectangle
			Win::Rect rect;
			if (regKey.GetValueLong ("Top", value))
				rect.top = value;
			else
				return;
			if (regKey.GetValueLong ("Left", value))
				rect.left = value;
			else
				return;
			if (regKey.GetValueLong ("Width", value))
				rect.right = rect.left + value;
			else
				return;
			if (regKey.GetValueLong ("Height", value))
				rect.bottom = rect.top + value;
			else
				return;
			placement.SetRect (rect);
			// flags
			if (regKey.GetValueLong ("Flags", value))
				placement.SetFlags (value);
			else
				return;
			// Corner when minimized
			Win::Point pt;
			if (regKey.GetValueLong ("XMin", value))
				pt.x = value;
			else
				return;
			if (regKey.GetValueLong ("YMin", value))
				pt.y = value;
			else
				return;
			placement.SetMinCorner (pt);
			// Corner when maximized
			if (regKey.GetValueLong ("XMax", value))
				pt.x = value;
			else
				return;
			if (regKey.GetValueLong ("YMax", value))
				pt.y = value;
			else
				return;
			placement.SetMinCorner (pt);
		}
	}

	// Global (recursive) functions
	void DeleteTree (RegKey::Handle key)
	{
		// Delete values
		std::vector<std::string>::iterator it;
		std::vector<std::string>::iterator end;
		std::vector<std::string> names;
		for (ValueSeq valueSeq (key); !valueSeq.AtEnd (); valueSeq.Advance ())
			names.push_back (valueSeq.GetName ());

		for (it = names.begin (); it != names.end (); ++it)
			key.DeleteValue (it->c_str ());

		names.clear ();
		// Delete sub-keys
		for (Seq keySeq (key); !keySeq.AtEnd (); keySeq.Advance ())
			names.push_back (keySeq.GetKeyName ());

		for (it = names.begin (); it != names.end (); ++it)
		{
			{
				RegKey::Existing subKey (key, it->c_str ());
				RegKey::DeleteTree (subKey);
			}
			key.DeleteSubKey (it->c_str ());
		}
	}

	void CopyTree (RegKey::Handle from, RegKey::Handle to)
	{
		// Copy values
		for (ValueSeq valueSeq (from); !valueSeq.AtEnd (); valueSeq.Advance ())
		{
			switch (valueSeq.GetType ())
			{
			case REG_DWORD:
				to.SetValueLong (valueSeq.GetName ().c_str (), valueSeq.GetLong ());
				break;
			case REG_SZ:
				to.SetValueString (valueSeq.GetName ().c_str (), valueSeq.GetString ());
				break;
			case REG_EXPAND_SZ:
				to.SetValueExpandedString (valueSeq.GetName ().c_str (), valueSeq.GetString ());
				break;
			case REG_MULTI_SZ:
				{
					MultiString mString;
					valueSeq.GetMultiString (mString);
					to.SetValueMultiString (valueSeq.GetName ().c_str (), mString);
				}
				break;
			case REG_BINARY:
				to.SetValueBinary (valueSeq.GetName ().c_str (), valueSeq.GetBytes (), valueSeq.GetValueLen ());
				break;
			default:
				throw Win::Exception ("Internal error: CopyTree -- unsupported registry value format");
				break;
			}
		}
		// Copy sub-key
		for (Seq keySeq (from); !keySeq.AtEnd (); keySeq.Advance ())
		{
			New toSubKey (to, keySeq.GetKeyName ());
			Existing fromSubKey (from, keySeq.GetKeyName ());
			RegKey::CopyTree (fromSubKey, toSubKey);
		}
	}

	// Specialized keys

	Existing::Existing (Handle keyParent, std::string const & subKeyName)
	{
		HKEY h;
		long err = ::RegOpenKeyEx (keyParent.ToNative (), 
								subKeyName.c_str (), 
								0,
								KEY_READ | KEY_WRITE, 
								&h);
		if (err != ERROR_SUCCESS)
		{
			throw Win::Exception ("Cannot open registry key.", 
								  ThePathMap.MakePath (keyParent.ToNative (), subKeyName).c_str (), 
								  err);
		}
		Reset (h);
		ThePathMap.Add (h, keyParent.ToNative (), subKeyName);
	}

	ReadOnly::ReadOnly (Handle keyParent, std::string const & subKeyName)
	{
		HKEY h;
		long err = ::RegOpenKeyEx (keyParent.ToNative (), 
								subKeyName.c_str (), 
								0,
								KEY_READ, 
								&h);
		if (err != ERROR_SUCCESS)
		{
			throw Win::Exception ("Registry operation failed - cannot open read-only key.",
									ThePathMap.MakePath (keyParent.ToNative (), subKeyName).c_str (), 
									err);
		}
		Reset (h);
		ThePathMap.Add (h, keyParent.ToNative (), subKeyName);
	}

	New::New (Handle keyParent, std::string const & subKeyName)
	{
		HKEY h;
		unsigned long disposition;
		long err = ::RegCreateKeyEx (keyParent.ToNative (), 
									subKeyName.c_str (), 
									0, 
									"",
									REG_OPTION_NON_VOLATILE, 
									KEY_READ | KEY_WRITE, 
									0,
									&h,
									&disposition);
		if (err != ERROR_SUCCESS)
		{
			throw Win::Exception ("Registry operation failed - cannot create key.",
									ThePathMap.MakePath (keyParent.ToNative (), subKeyName).c_str (), 
									err);
		}
		Reset (h);
		ThePathMap.Add (h, keyParent.ToNative (), subKeyName);
	}

	Check::Check (Handle keyParent, std::string const & subKeyName, bool writeable)
		: _exists (false)
	{
		if (!keyParent.IsNull ())
		{
			HKEY hKey;
			_exists = (::RegOpenKeyEx (keyParent.ToNative (), 
									subKeyName.c_str (), 
									0, 
									writeable ? KEY_READ | KEY_WRITE : KEY_READ, 
									&hKey) == ERROR_SUCCESS);
			if (_exists)
			{
				Reset (hKey);
				ThePathMap.Add (hKey, keyParent.ToNative (), subKeyName);
			}
		}
	}

	std::string System::GetProgramFilesPath ()
	{
	    ReadOnly keyWindows (_keyMicrosoft, "Windows");
		ReadOnly keyCurVer  (keyWindows,    "CurrentVersion");

		return keyCurVer.GetStringVal ("ProgramFilesDir"); 
	}
	std::string System::GetRegisteredOwner ()
	{
		SystemVersion osVer;
		bool isNT = osVer.IsOK () && osVer.IsWinNT ();

		ReadOnly keyWindows (_keyMicrosoft, isNT ? "Windows NT" : "Windows");
		ReadOnly keyCurVer  (keyWindows, "CurrentVersion");

		return keyCurVer.GetStringVal ("RegisteredOwner");
	}

	ValueSeq::ValueSeq (RegKey::Handle key)
		: _key (key.ToNative ()),
		  _cur (0xffffffff)
	{
		_status = ::RegQueryInfoKey (_key,       // handle of key to query 
									 0,          // address of buffer for class string 
									 0,          // address of size of class string buffer 
									 0,          // reserved 
									 0,          // address of buffer for number of sub-keys 
									 0,          // address of buffer for longest sub-key name length  
									 0,          // address of buffer for longest class string length 
									 &_count,    // address of buffer for number of value entries 
									 &_nameLen,  // address of buffer for longest value name length 
									 &_valueLen, // address of buffer for longest value data length 
									 0,          // address of buffer for security descriptor length 
									 0);         // address of buffer for last write time 
		if (_status != ERROR_SUCCESS)
			throw Win::Exception ("Registry error: Cannot get key info",
									ThePathMap.GetPath (key.ToNative ()).c_str (), 
									_status);
		_name.resize (_nameLen + 1);
		_value.resize (_valueLen + 1);
		Advance ();
	}

	void ValueSeq::Advance ()
	{
		_nameLen = _name.length ();
		_valueLen = _value.size ();
		_cur++;
		_status = ::RegEnumValue(_key,       // handle of key to query 
								 _cur,       // index of value to query 
								 &_name [0], // address of buffer for value string 
								 &_nameLen,  // address for size of value buffer 
								 0,          // reserved 
								 &_type,     // address of buffer for type code 
								 &_value [0],// address of buffer for value data 
								 &_valueLen);// address for size of data buffer 
	}

	std::string ValueSeq::GetName () const
	{
		return _name.substr (0, _nameLen);
	}

	std::string ValueSeq::GetString () const
	{
		Assert (IsString ());
		Assert (_valueLen >= 1);
		// _valueLen includes terminating '\0'
		return std::string (reinterpret_cast<char const *>(&_value [0]), _valueLen - 1);
	}

	void ValueSeq::GetMultiString (MultiString & mString) const
	{
		Assert (IsMultiString ());
		Assert (_valueLen >= 1);
		// _valueLen includes terminating '\0'
		mString.Assign (reinterpret_cast<char const *>(&_value [0]), _valueLen - 1);
	}

	Seq::Seq (RegKey::Handle key)
		: _key (key.ToNative ()),
		  _cur (-1)
	{
		unsigned long maxNameLen = 0;
		unsigned long maxClassLen = 0;
		_status = ::RegQueryInfoKey (_key,     // handle of key to query 
									0,         // address of buffer for class string 
									0,         // address of size of class string buffer 
									0,         // reserved 
									&_count,   // address of buffer for number of sub-key 
									&maxNameLen, // address of buffer for longest sub-key name length  
											   // NT/2000/XP:  The size does not include the terminating null character.
											   // Me/98/95:  The size includes the terminating null character.
									&maxClassLen,// address of buffer for longest class string length 
											   // The returned value does not include the terminating null character.
									0,         // address of buffer for number of value entries 
									0,         // address of buffer for longest value name length 
									0,         // address of buffer for longest value data length 
									0,         // address of buffer for security descriptor length 
									0);        // address of buffer for last write time 
		if (_status != ERROR_SUCCESS)
			throw Win::Exception ("Registry error: Cannot get key info:",
									ThePathMap.GetPath (key.ToNative ()).c_str (),
									_status);
		maxNameLen += 1; // instead of checking the Windows version just make maxNameLen one byte longer
		maxClassLen += 1; // add a place for the terminating null character
		_name.resize (maxNameLen);
		_class.resize (maxClassLen);

		Advance ();
	}

	void Seq::Advance ()
	{
		_cur++;
		_nameLen = _name.size ();
		_classLen = _class.size ();
		_status = ::RegEnumKeyEx (_key,       // handle of key to query 
								  _cur,       // index of sub-key to enumerate 
								  &_name [0], // address of buffer for sub-key name 
								  &_nameLen,  // address for size of sub-key buffer 
								  0,          // reserved 
								  &_class [0],// address of buffer for class string 
								  &_classLen, // address for size of class buffer 
								  &_lastTime);// address for time key last written to 
		Assert (_nameLen <= _name.size ());
		Assert (_classLen <= _class.size ());
	}
}

#if !defined (NDEBUG) && !defined (BETA)

#include <File/Path.h>
#include <auto_Vector.h>

// RegKey unit test
namespace UnitTest
{
	class RegKeyTester
	{
	public:
		RegKeyTester (std::string const & valueName)
			: _valueName (valueName)
		{}

		std::string const & GetValueName () const { return _valueName; }

		virtual bool Write (RegKey::Handle key) = 0;
		virtual void Test (RegKey::ValueSeq & seq, RegKey::Handle key) = 0;

	private:
		std::string	_valueName;
	};

	class TestLong : public RegKeyTester
	{
	public:
		TestLong (std::string const & valueName, unsigned long testValue)
			: RegKeyTester (valueName),
			_testValue (testValue)
		{}

		bool Write (RegKey::Handle key)
		{
			return key.SetValueLong (GetValueName (), _testValue);
		}
		void Test (RegKey::ValueSeq & seq, RegKey::Handle key)
		{
			Assert (seq.IsLong ());
			unsigned long value = seq.GetLong ();
			Assert (value == _testValue);
			Assert (key.GetValueLong (GetValueName (), value));
			Assert (value == _testValue);
		}

	private:
		unsigned long	_testValue;
	};

	class TestString : public RegKeyTester
	{
	public:
		TestString (std::string const & valueName, std::string const & testValue)
			: RegKeyTester (valueName),
			_testString (testValue)
		{}

		bool Write (RegKey::Handle key)
		{
			return key.SetValueString (GetValueName (), _testString);
		}
		void Test (RegKey::ValueSeq & seq, RegKey::Handle key)
		{
			Assert (seq.IsString ());
			std::string value = seq.GetString ();
			Assert (value == _testString);
			value = key.GetStringVal (GetValueName ());
			Assert (value == _testString);
		}

	private:
		std::string	_testString;
	};

	class TestMultiString : public RegKeyTester
	{
	public:
		TestMultiString (std::string const & valueName, std::vector<std::string> const & testValues)
			: RegKeyTester (valueName)
		{
			std::copy (testValues.begin (), testValues.end (), std::back_inserter (_testMultiString));
		}

		void push_back (std::string const & str) { _testMultiString.push_back (str); }

		bool Write (RegKey::Handle key)
		{
			return key.SetValueMultiString (GetValueName (), _testMultiString);
		}
		void Test (RegKey::ValueSeq & seq, RegKey::Handle key)
		{
			Assert (seq.IsMultiString ());

			MultiString mString;
			seq.GetMultiString (mString);
			MultiString::const_iterator iter1 = mString.begin ();
			MultiString::const_iterator iter2 = _testMultiString.begin ();
			for (; iter1 != mString.end () && iter2 != _testMultiString.end (); ++iter1, ++iter2)
			{
				Assert (strcmp (*iter1, *iter2) == 0);
			}
			Assert (iter1 == mString.end () && iter2 == _testMultiString.end ());

			MultiString mString1;
			key.GetMultiString (GetValueName (), mString1);
			iter1 = mString1.begin ();
			iter2 = _testMultiString.begin ();
			for (; iter1 != mString1.end () && iter2 != _testMultiString.end (); ++iter1, ++iter2)
			{
				Assert (strcmp (*iter1, *iter2) == 0);
			}
			Assert (iter1 == mString1.end () && iter2 == _testMultiString.end ());
		}

	private:
		MultiString	_testMultiString;
	};

	class IsEqualName : public std::unary_function<RegKeyTester const *, bool>
	{
	public:
		IsEqualName (std::string const & valueName)
			: _valueName (valueName)
		{}

		bool operator () (RegKeyTester const * tester) const
		{
			return tester->GetValueName () == _valueName;
		}

	private:
		std::string const &	_valueName;
	};

	void WriteTestValues (auto_vector<RegKeyTester> const & testers, RegKey::Handle key)
	{
		for (auto_vector<RegKeyTester>::const_iterator iter = testers.begin ();
			iter != testers.end ();
			++iter)
		{
			Assert ((*iter)->Write (key));
		}
	}

	void TestValues (auto_vector<RegKeyTester> const & testers, RegKey::Handle key)
	{
		RegKey::ValueSeq seq (key);
		Assert (seq.Count () == testers.size ());
		for (; !seq.AtEnd (); seq.Advance ())
		{
			std::string valueName = seq.GetName ();
			auto_vector<RegKeyTester>::const_iterator tester =
				std::find_if (testers.begin (), testers.end (), IsEqualName (valueName));
			Assert (tester != testers.end ());
			(*tester)->Test (seq, key);
		}
	}

	void CreateSeqTestKeys (RegKey::Handle root, std::vector<std::string> const & subKeys)
	{
		for (unsigned int i = 0; i < subKeys.size (); ++i)
		{
			RegKey::New testSubKey (root, subKeys [i]);
			RegKey::Check testSubKeyCheck (root, subKeys [i]);
			Assert (testSubKeyCheck.Exists ());
		}
	}

	void TestSeq (RegKey::Handle root, std::vector<std::string> const & subKeys)
	{
		unsigned int keyCount = subKeys.size ();
		RegKey::Seq seq (root);
		for (unsigned int trial = 0; trial < 3; ++trial)
		{
			for (unsigned int i = 0; i < keyCount; ++i)
			{
				Assert (!seq.AtEnd ());
				Assert (seq.Count () == keyCount);
				Assert (seq.GetKeyName () == subKeys [i]);
				seq.Advance ();
			}
			Assert (seq.AtEnd ());
			Assert (seq.Count () == keyCount);
			seq.Rewind ();
		}
	}

	void RegistryTest (std::ostream & out)
	{
		out << "Registry test" << std::endl;
		FilePath testKeyPath ("Software\\Reliable Software\\Code Co-op");
		RegKey::CurrentUser root;

		auto_vector<RegKeyTester> testers;
		std::unique_ptr<RegKeyTester> test (new TestLong ("Test long", 125));
		testers.push_back (std::move(test));

		test.reset (new TestString ("Test string", "AbcXyz."));
		testers.push_back (std::move(test));

		std::vector<std::string> testValues;
		test.reset (new TestMultiString ("Test multi string (0)", testValues));
		testers.push_back (std::move(test));
		testValues.push_back ("Abc");
		test.reset (new TestMultiString ("Test multi string (1)", testValues));
		testers.push_back (std::move(test));
		testValues.push_back ("Xyz");
		test.reset (new TestMultiString ("Test multi string (2)", testValues));
		testers.push_back (std::move(test));

		{
			// Writing and reading values
			RegKey::New testKey1 (root, testKeyPath.GetFilePath ("Test1"));
			RegKey::Check testKey1Check (root, testKeyPath.GetFilePath ("Test1"));
			Assert (testKey1Check.Exists ());

			WriteTestValues (testers, testKey1);
			TestValues (testers, testKey1);

			// Create sub-key under Test1
			RegKey::New testKey2 (testKey1, "Test2");
			WriteTestValues (testers, testKey2);
			RegKey::New testKey3 (testKey2, "Test3");

			// Copy Test1 tree to the Test4 tree
			RegKey::New testKey4 (root, testKeyPath.GetFilePath ("Test4"));
			RegKey::CopyTree (testKey1, testKey4);
			TestValues (testers, testKey4);
			RegKey::Existing testKey4SubKey (testKey4, "Test2");
			TestValues (testers, testKey4SubKey);
		}

		// Deleting keys and values
		{
			RegKey::Existing testKey1 (root, testKeyPath.GetFilePath ("Test1"));
			RegKey::DeleteTree (testKey1);
			Assert (testKey1.IsKeyEmpty ());
			RegKey::Existing testKey4 (root, testKeyPath.GetFilePath ("Test4"));
			RegKey::DeleteTree (testKey4);
			Assert (testKey4.IsKeyEmpty ());

			RegKey::Existing testRoot (root, testKeyPath.GetDir ());
			testRoot.DeleteSubKey ("Test1");
			testRoot.DeleteSubKey ("Test4");
		}

		RegKey::Check testKey1Check (root, testKeyPath.GetFilePath ("Test1"));
		Assert (!testKey1Check.Exists ());

		RegKey::Check testKey4Check (root, testKeyPath.GetFilePath ("Test4"));
		Assert (!testKey4Check.Exists ());
	
		// Iteration over sub keys
		{
			char const SeqTestKeyName [] = "SubKey Seq Test";
			RegKey::Existing allTestsRoot (root, testKeyPath.GetDir ());
			{
				RegKey::Check seqTestRootCheck (root, testKeyPath.GetFilePath (SeqTestKeyName));
				if (seqTestRootCheck.Exists ())
				{
					RegKey::Existing seqTestRoot (root, testKeyPath.GetFilePath (SeqTestKeyName));
					RegKey::DeleteTree (seqTestRoot);
					Assert (seqTestRoot.IsKeyEmpty ());
					allTestsRoot.DeleteSubKey (SeqTestKeyName);
				}
			}
			{
				RegKey::New seqTestRoot (root, testKeyPath.GetFilePath (SeqTestKeyName));
				RegKey::Check seqTestRootCheck (root, testKeyPath.GetFilePath (SeqTestKeyName));
				Assert (seqTestRootCheck.Exists ());
				std::vector<std::string> subKeys;

				{
					// empty key
					subKeys.clear ();
					CreateSeqTestKeys (seqTestRoot, subKeys);
					TestSeq (seqTestRoot, subKeys);
				}
				{
					// one sub key
					subKeys.clear ();
					subKeys.push_back ("SubKey 1");
					CreateSeqTestKeys (seqTestRoot, subKeys);
					TestSeq (seqTestRoot, subKeys);
				}
				{
					// multiple sub keys
					subKeys.clear ();
					subKeys.push_back ("SubKey 1");
					subKeys.push_back ("SubKey 2");
					subKeys.push_back ("SubKey 3");
					subKeys.push_back ("SubKey 4");
					subKeys.push_back ("SubKey 5");
					CreateSeqTestKeys (seqTestRoot, subKeys);
					TestSeq (seqTestRoot, subKeys);
				}
				RegKey::DeleteTree (seqTestRoot);
				Assert (seqTestRoot.IsKeyEmpty ());
			}
			allTestsRoot.DeleteSubKey (SeqTestKeyName);
		}
		out << "Passed!" << std::endl;
	}
}

#endif
