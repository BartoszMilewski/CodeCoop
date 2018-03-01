#if !defined (PREFERENCESSTORAGE_H)
#define PREFERENCESSTORAGE_H
//------------------------------------
//  (c) Reliable Software, 2005 - 2008
//------------------------------------

#include <Sys/RegKey.h>
#include <File/Path.h>
#include <Win/Win.h>

namespace Win
{
	class ClientRect;
}

class MultiString;
class NamedBool;

namespace Preferences
{
	class Storage
	{
	public:
		class Sequencer
		{
		public:
			Sequencer (Preferences::Storage const & storage)
				: _key (storage._root, storage._keyPath.GetDir ()),
				  _seq (_key)
			{}

			bool AtEnd () const { return _seq.AtEnd (); }
			void Advance () { _seq.Advance (); }

			std::string GetValueName () const { return _seq.GetName (); }
			unsigned long GetValueLong () const { return _seq.GetLong (); }
			std::string GetValueString () const { return _seq.GetString (); }
			void GetMultiString (MultiString & mString) const { return _seq.GetMultiString (mString); }
			void GetAsNamedBool (NamedBool & options) const;
			bool IsValueLong () const { return _seq.IsLong (); }
			bool IsValueString () const { return _seq.IsString (); }
			bool IsMultiValue () const { return _seq.IsMultiString (); }

		private:
			RegKey::Existing	_key;
			RegKey::ValueSeq	_seq;
		};

		friend class Sequencer;

	public:
		// regKeyPath is relative to the HKEY_CURRENT_USER key
		Storage (std::string const & regKeyPath);
		// keyName is relative to the path already remembered by the storage
		Storage (Preferences::Storage const & storage, std::string const & keyName);

		bool PathExists () const;
		void CreatePath ();

		bool IsValuePresent (std::string const & valueName) const;

		void Read (std::string const & valueName, unsigned long & value) const;
		void Read (std::string const & valueName, std::string & value) const;
		void Read (std::string const & valueName, MultiString & value) const;
		void Read (Win::Placement & placement) const;
		void Read (std::string const & valueName, NamedBool & options) const;

		void Save (std::string const & valueName, unsigned long value) throw ();
		void Save (std::string const & valueName, std::string const & value) throw ();
		void Save (std::string const & valueName, MultiString const & value) throw ();
		void Save (Win::Placement const & placement) throw ();
		void Save (std::string const & valueName, NamedBool const & options) throw ();

		virtual void Verify () = 0;

		int GetCharCx () const { return _cxChar; }

	private:
		void RememberSystemMetrics ();

	private:
		RegKey::CurrentUser	_root;
		FilePath			_keyPath;

	protected:
		// Helpful information used during preferences verification
		int					_cxChar;
		int					_cyChar;
		int					_cyCaption;
	};

	class TopWinPlacement : public Preferences::Storage
	{
		enum 
		{
			TopWindowMinWidth = 200,
			TopWindowMinHeight = 300
		};
	public:
		TopWinPlacement (Win::Dow::Handle win, std::string const & path);
		~TopWinPlacement ();

		void Verify ();
		void PlaceWindow (Win::ShowCmd cmdShow = Win::Show, bool multipleWindows = false);
		void Update ();

	private:
		void SetDefaultPositionAndSize (Win::ClientRect const & screenRect);

	private:
		Win::Dow::Handle	_topWin;
		Win::Placement		_placement;
	};

}

#endif
