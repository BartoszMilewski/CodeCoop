#if !defined (EDIT_H)
#define EDIT_H
//--------------------------------
// (c) Reliable Software 1998-2002
//--------------------------------

#include "Controls.h"

namespace Win
{
	class EditReadOnly: public SimpleControl
	{
	public:
		EditReadOnly () {}
		EditReadOnly (Win::Dow::Handle winParent, int id)
			: SimpleControl (winParent, id)
		{}

		void SetString (char const * buf)
		{
			SetText (buf);
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
			Edit::Style & operator<<(void (Edit::Style::*method)())
			{
				(this->*method)();
				return *this;
			}
			void MultiLine ()
			{
				_style |= ES_MULTILINE;
			}
			void ReadOnly ()
			{
				_style |= ES_READONLY;
			}
			void AutoHScroll ()
			{
				_style |= ES_AUTOHSCROLL;
			}
			void AutoVScroll ()
			{
				_style |= ES_AUTOVSCROLL;
			}
			void AlignCenter ()
			{
				_style |= ES_CENTER;
			}
			void AlignLeft ()
			{
				_style |= ES_LEFT;
			}
			void AlignRight ()
			{
				_style |= ES_RIGHT;
			}
			void WantReturn ()
			{
				_style |= ES_WANTRETURN;
			}
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

		//	for selecting the end position
		unsigned short EndPos() const
		{
			return 0xffff;
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

		// Notifications
		static bool IsKillFocus (int notifyCode) { return notifyCode == EN_KILLFOCUS; }
	};

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
