#if !defined (EDIT_H)
#define EDIT_H
//----------------------------------
// (c) Reliable Software 1998 - 2007
//----------------------------------

#include "Controls.h"

#include <Graph/Canvas.h>
#include <ostream>

namespace Win
{
	class EditReadOnly: public SimpleControl
	{
	public:
		EditReadOnly () {}
		EditReadOnly (Win::Dow::Handle winParent, int id)
			: SimpleControl (winParent, id)
		{}

		void SetString (std::string const & buf)
		{
			SetText (buf.c_str ());
		}
		void Select (int offStart, int offEnd)
		{
			SendMsg (EM_SETSEL, (WPARAM) offStart, (LPARAM) offEnd);
		}
	};

	class Edit: public SimpleControl
	{
	public:
		class Style : public Win::Style
		{
		public:
			enum Bits
			{
				MultiLine = ES_MULTILINE,
				ReadOnly = ES_READONLY,
				AutoHScroll = ES_AUTOHSCROLL,
				AutoVScroll = ES_AUTOVSCROLL,
				AlignCenter = ES_CENTER,
				AlignLeft = ES_LEFT,
				AlignRight = ES_RIGHT,
				WantReturn = ES_WANTRETURN
			};
		};

	public:
		Edit () {}
		Edit (Win::Dow::Handle winParent, int id)
			: SimpleControl (winParent, id)
		{}

		void SetString (char const * buf)
		{
			SetText (buf);
		}

		void SetString (std::string const & str)
		{
			SetText (str.c_str ());
		}

		void Append (char const * buf);
		void Append (std::string const & str)
		{
			Append (str.c_str ());
		}

		//	for selecting the end position
		unsigned short EndPos() const
		{
			return std::numeric_limits<unsigned short>::max();
		}

		void Select (int offStart, int offEnd)
		{
			SendMsg (EM_SETSEL, (WPARAM) offStart, (LPARAM) offEnd);
		}

		void SetReadonly (bool flag)
		{
			SendMsg (EM_SETREADONLY, (WPARAM) (flag ? TRUE : FALSE), 0);
		}

		void LimitText (int limit)
		{
			SendMsg (EM_LIMITTEXT, limit);
		}

		// code is the HIWORD (wParam)
		static bool IsChanged (int code)
		{ 
			return code == EN_CHANGE;
		}

		static bool GotFocus (int code)
		{ 
			return code == EN_SETFOCUS;
		}

		int GetLen () const
		{
			return SendMsg (WM_GETTEXTLENGTH);
		}

		int GetLineCount () const
		{
			return SendMsg (EM_GETLINECOUNT);
		}

		bool GetModify() const
		{
			return SendMsg(EM_GETMODIFY) != 0;
		}

		void ClearModify()
		{
			SendMsg(EM_SETMODIFY, FALSE);
		}

		std::string GetString () const;
		std::string GetTrimmedString () const;

		void GetString (char * buf, int len) const
		{
			SendMsg (WM_GETTEXT, (WPARAM) len, (LPARAM) buf);
		}

		bool GetInt (int & value);
		bool GetUnsigned (unsigned & value);

		void Select ()
		{
			SendMsg (EM_SETSEL, 0, -1);
		}

		void SelectLine (int lineIdx);
		void ReplaceSelection (char const * info)
		{
			SendMsg (EM_REPLACESEL, 0, reinterpret_cast<LPARAM>(info));
		}
		void Clear ();

		int CharFromPos(int x, int y, int& line) const
		{
			LRESULT lresult = SendMsg(EM_CHARFROMPOS, 0, MAKELPARAM(x, y));
			line = HIWORD(lresult);
			return LOWORD(lresult);
		}

		int CharFromPos(int x, int y) const	//	for when you don't need the line #
		{
			int unused;
			return CharFromPos(x, y, unused);
		}

		//	only for multiline
		void SetRect(const Win::Rect& rc)
		{
			SendMsg(EM_SETRECT, 0, reinterpret_cast<LPARAM> (&rc));
		}
		void SetRectNoPaint (Win::Rect const & rect)
		{
			SendMsg (EM_SETRECTNP, 0, reinterpret_cast<LPARAM>(&rect));
		}
		void LineScroll (unsigned int howManyLines = 1)
		{
			SendMsg (EM_LINESCROLL, 0, howManyLines);
		}

		// Notifications
		static bool IsKillFocus (int notifyCode) { return notifyCode == EN_KILLFOCUS; }
	};


	class StreamEdit : public Win::Edit, public std::streambuf
	{
	public:
		~StreamEdit () {}

		// streambuf
		int sync ();
		int overflow (int nCh = EOF);
		int doallocate ();
	private:
		//	nBufferSize doesn't really matter;  we'll flush if we need more room
		enum { nPutAreaSize = 1024 };
		//	The buffer needs an extra character so we can append a NULL to the data
		char _buffer [nPutAreaSize + 1];
	};

	inline Win::Style & operator<< (Win::Style & style, Win::Edit::Style::Bits bits)
	{
		style.OrIn (static_cast<Win::Style::Bits> (bits));
		return style;
	}

	class EditMaker : public ControlMaker
	{
	public:
		EditMaker(Win::Dow::Handle winParent, int id)
			: ControlMaker("EDIT", winParent, id)
		{
		}
		Win::Edit::Style & Style () { return static_cast<Win::Edit::Style &> (_style); }
	};
}

#endif
