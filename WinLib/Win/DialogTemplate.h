#if !defined (DIALOGTEMPLATE_H)
#define DIALOGTEMPLATE_H
//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include <Win/Geom.h>
#include <Graph/Font.h>
#include <Ctrl/Static.h>

namespace Dialog
{
	class Template
	{
	private:
		// This is the beginning of the extended dialog template
		struct Ex {
			WORD dlgVer;
			WORD signature;
			DWORD helpID;
			DWORD exStyle;
			DWORD style;
			WORD cDlgItems;
			short x;
			short y;
			short cx;
			short cy;
		};
	public:
		class Serializer
		{
		public:
			Serializer (std::vector<unsigned char> & buf)
				: _buf (buf)
			{}
			void DwordAlign ()
			{
				unsigned size = _buf.size ();
				int delta = size % sizeof (DWORD);
				if (delta != 0)
					Extend (sizeof (DWORD) - delta);
			}
			void PutWord (unsigned short x)
			{
				void * p = Extend (sizeof (WORD));
				* static_cast<WORD *> (p) = x;
			}
			void PutDWord (unsigned long x)
			{
				void * p = Extend (sizeof (DWORD));
				* static_cast<DWORD *> (p) = x;
			}
			void PutShort (short x)
			{
				void * p = Extend (sizeof (short));
				* static_cast<short *> (p) = x;
			}
			void PutByte (unsigned char x)
			{
				_buf.push_back (x);
			}
			void PutString (std::wstring const & str)
			{
				unsigned len = str.size ();
				for (unsigned n = 0; n < len; ++n)
					PutWchar (str [n]);
				PutWchar (0);
			}
			void PutWchar (wchar_t c)
			{
				void * p = Extend (sizeof (WCHAR));
				* static_cast<WCHAR *> (p) = c;
			}

		private:
			void * Extend (int n)
			{
				unsigned s = _buf.size ();
				_buf.resize (s + n);
				return &_buf[s];
			}
			std::vector<unsigned char> & _buf;
		};

	public:
		Template () 
			: _version (0) 
		{}
		void Load (Win::Instance inst, int dlgId);
		Dialog::Template::Serializer GetSerializer ()
		{
			return Serializer (_data);
		}
		DLGTEMPLATE const * ToNative () const 
		{ 
			return reinterpret_cast<DLGTEMPLATE const *> (&_data[0]); 
		}
		int Width () const { return (_version == 1)? ToEx ()->cx: ToTmpl ()->cx; }
		int Height () const { return (_version == 1)? ToEx ()->cy: ToTmpl ()->cy; }
		void SetVersion (int version) { _version = version; }
	private:
		DLGTEMPLATE const * ToTmpl () const 
		{ 
			return reinterpret_cast<DLGTEMPLATE const *> (&_data[0]); 
		}
		Template::Ex const * ToEx () const 
		{ 
			return reinterpret_cast<Ex const *> (&_data[0]); 
		}
	private:
		std::vector<unsigned char>	_data;
		int							_version;
	};

	class TemplateMaker
	{
		class Item 
		{
			friend class TemplateMaker;
		public:
			enum Class
			{
				ButtonClass = 0x0080,
				EditClass = 0x0081,
				StaticClass = 0x0082,
				ListBoxClass = 0x0083,
				ScrollBarClass = 0x0084,
				ComboBoxClass = 0x0085
			};
		public:
			void SetRect (Win::Rect const & rect)
			{
				_rect = rect;
			}
			void SetRect (int x, int y, int cx, int cy)
			{
				Win::Rect r (x, y, x + cx, y + cy);
				SetRect (r);
			}
			void SetClassName (std::wstring const & name)
			{
				_className = name;
			}
			void SetText (std::wstring const & txt)
			{
				_text = txt;
			}

			Win::Style & Style () { return _style; }

		protected:
			Item (unsigned id) 
				: _helpId (0)
				, _id (id)
			{
				_style << Win::Style::Visible
					   << Win::Style::Child;
			}

			void SetClass (Class cl) { _classId = cl; }

			void Serialize (Dialog::Template::Serializer & out) const;

		private:
			unsigned		_id;
			std::wstring	_text;
			unsigned		_helpId;
			Win::Style		_style;
			Win::Rect		_rect;
			Class			_classId;
			std::wstring	_className;
		};

	public:
		class Button: public Item
		{
		public:
			Button (unsigned id)
				: Item (id)
			{
				_style << Win::Style::Tabstop;
				SetClass (ButtonClass);
			}
		};

		class StaticText: public Item
		{
		public:
			StaticText (unsigned id = -1) // IDC_STATIC
				: Item (id)
			{
				SetClass (StaticClass);
				_style << Win::Style::Group;
			}
			void Center () 
			{
				_style << Win::Static::Style::AlignCenter;
			}
		};

		class ProgressBar : public Item
		{
		public:
			ProgressBar (unsigned id)
				: Item (id)
			{
				_style << Win::Style::AddBorder;
				SetClassName (L"msctls_progress32");
			}
		};

	private:
		typedef std::vector<Item const *> ItemList;

	public:
		TemplateMaker ();
		void SetRect (Win::Rect const & rect)
		{
			_rect = rect;
		}
		void SetFont (std::wstring const & name,
					  unsigned pointSize,
					  unsigned weight,
					  bool italic,
					  unsigned charSet);
		void SetTitle (std::wstring const & title)
		{
			_title = title;
		}
		void AddItem (Item const * item)
		{
			_items.push_back (item);
		}
		void Create (Dialog::Template & tmpl);

	public:
		unsigned		_helpId;
		Win::Style		_style;
		Win::Rect		_rect;
		std::wstring	_title;
		std::wstring	_fontName;
		unsigned		_pointSize;
		unsigned		_weight;
		bool			_italic;
		unsigned		_charSet;
		ItemList		_items;
	};
}

#endif
