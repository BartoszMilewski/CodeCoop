#if !defined (COMBOBOX_H)
#define COMBOBOX_H
//------------------------------------
//  (c) Reliable Software, 1998 - 2007
//------------------------------------

#include <Ctrl/Controls.h>
#include <Graph/Font.h>
#include <Ctrl/Edit.h>
#include <Win/Utility.h>

namespace Win
{
	class ComboBox : public Win::ControlWithFont
	{
#if !defined NDEBUG
	friend std::ostream& operator<<(std::ostream& os, ComboBox& cbo);
#endif
	public:
		class Style : public Win::Style
		{
		public:
			enum Bits
			{
				Simple = CBS_SIMPLE,
				DropDown = CBS_DROPDOWN,
				DropDownList = CBS_DROPDOWNLIST,
				AddSort = CBS_SORT,
				HasStrings = CBS_HASSTRINGS,
				AutoScroll = CBS_AUTOHSCROLL,
				NoIntegralHeight = CBS_NOINTEGRALHEIGHT,

				TypeMask = CBS_SIMPLE | CBS_DROPDOWN | CBS_DROPDOWNLIST
			};
		public:
		};
	private:
#if defined(TESTING)
		//	Note that GetComboBoxInfo is not available on
		//	Windows 95 or Windows NT 4.0 pre SP6
		class ComboBoxInfo : public COMBOBOXINFO
		{
		public:
			//	Use pointer to make it easy to pass 'this'
			ComboBoxInfo(Win::ComboBox const *pcbo)
			{
				cbSize = sizeof(COMBOBOXINFO);
				BOOL ret = ::GetComboBoxInfo(pcbo->ToNative (), this);
				Assert(ret != 0);
			}
		};

		class WindowInfo : public Win::WindowInfo
		{
		public:
			WindowInfo(Win::ComboBox cbo)
				: Win::WindowInfo(cbo)
			{}
			bool IsDropDown() const
			{
				return (dwStyle & (CBS_DROPDOWN | CBS_DROPDOWNLIST | CBS_SIMPLE)) == CBS_DROPDOWN;
			}
		};
#endif
	public:
		class NotifyHandler
		{
		public:
			~NotifyHandler () {}

			virtual void OnSelEndOk () {}
			virtual void OnSelEndCancel () {}
			virtual void OnKillFocus () {}
			virtual void OnEditChange () {}
			virtual void OnDblClick () {}
			virtual void OnCloseUp () {}
			virtual void OnDropDown () {}
			virtual void OnEditUpdate () {}
			virtual void OnSetFocus () {}
			virtual void OnSelChange () {}
		};

	public:
		ComboBox (Win::Dow::Handle winParent, int id)
			: Win::ControlWithFont (winParent, id) 
		{}
		ComboBox (Win::Dow::Handle win)
			: Win::ControlWithFont (win)
		{}
		ComboBox ()
		{}
		void OnNotify (unsigned int code);

		void ReSize (int left, int top, int width, int height)
		{
			Move (left, top, width, height);
		}

		void Display (char const * info);

		int AddToList (std::string const & item)
		{
			return SendMsg (CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item.c_str ()));
		}

		int Insert (int idx, char const * item) 
		{
			return SendMsg (CB_INSERTSTRING, idx, reinterpret_cast<LPARAM>(item));
		}

		bool SetItemData (int idx, unsigned long data)
		{
			return (SendMsg (CB_SETITEMDATA, idx, data) != CB_ERR);
		}

		unsigned long GetItemData (int idx)
		{
			return SendMsg (CB_GETITEMDATA, idx, 0);
		}

		int Remove (int idx)
		{
			return SendMsg (CB_DELETESTRING, idx, 0);
		}
		int FindString (char const * string , int idxBegin = 0) const
		{
			int idFind =  SendMsg (CB_FINDSTRING, idxBegin, reinterpret_cast<LPARAM>(string));
			if (idFind != CB_ERR)
				return idFind;
			return -1;
		}
		int FindStringExact (char const * string , int idxBegin = 0) const
		{
			int idFind =  SendMsg (CB_FINDSTRINGEXACT, idxBegin, reinterpret_cast<LPARAM>(string));
			if (idFind != CB_ERR)
				return idFind;
			return -1;
		}

		void SetSelection (int idx)
		{
			SendMsg (CB_SETCURSEL, idx, 0);
		}
		
		int Count () const
		{
			return SendMsg (CB_GETCOUNT, 0, 0);
		}

		void DropDown (bool show)
		{
			SendMsg (CB_SHOWDROPDOWN, show ? TRUE : FALSE, 0);
		}

		long GetHeight () const;

		int GetEditTextLength () const
		{
			return SendMsg (WM_GETTEXTLENGTH, 0, 0);
		}

		std::string RetrieveEditText () const;
		std::string RetrieveTrimmedEditText () const;

		void SetEditText (char const * str)
		{
			SendMsg (WM_SETTEXT, 0, reinterpret_cast<LPARAM>(str));
		}

		int GetTextLength (int idx) const
		{
			return SendMsg (CB_GETLBTEXTLEN, idx, 0);
		}
        // function no case sensitive
		bool SelectString (char const * str)
		{
			int result = SendMsg (CB_SELECTSTRING, -1, reinterpret_cast<LPARAM> (str));
			return result != CB_ERR;
		}

		std::string RetrieveText (int idx) const;

		int GetSelectionIdx () const
		{
			return SendMsg (CB_GETCURSEL, 0, 0);
		}

		Win::Edit GetEditWindow() const
		{
			//	Combo boxes have exactly one child window - the edit window (the
			//	down list is a child of the desktop window).  Use FindWindowEx to access
			//	the child edit window (because GetComboBoxInfo is not supported on
			//	Windows 95 or Windows NT 4.0 pre SP6
			Win::Edit edit;
			edit.Reset(::FindWindowEx(H (), 0, 0, 0));
			return edit;
		}

		int GetItemHeight(int idx = 0) const
		{
			return SendMsg(CB_GETITEMHEIGHT, idx);
		}

		//	Given # of drop down rows return height to set control
		//  (because Windows uses the passed height for both the combobox and
		//	the drop down list)
		int RowsToHeight(int nRows) const;

		void SetItemHeight(int height, int idx = 0)
		{
			SendMsg(CB_SETITEMHEIGHT, idx, height);
		}

		void Empty ()
		{
			SendMsg (CB_RESETCONTENT);
		}

		void SetNotifyHandler (std::unique_ptr<NotifyHandler> handler) { _notifyHandler = std::move(handler);}

		static bool IsSelectionChange (unsigned notifyCode) { return notifyCode == CBN_SELCHANGE; }
		static bool IsEditChange (unsigned notifyCode) { return notifyCode == CBN_EDITCHANGE; }

	private:
		std::unique_ptr<NotifyHandler> _notifyHandler;
	};

	inline Win::Style & operator<<(Win::Style & style, ComboBox::Style::Bits bits)
	{
		style.OrIn (static_cast<Win::Style::Bits> (bits));
		return style;
	}

#if !defined NDEBUG
	std::ostream& operator<<(std::ostream& os, ComboBox& cbo);
#endif

	class ComboBoxMaker : public Win::ControlMaker
	{
	public:
		ComboBoxMaker (Win::Dow::Handle winParent, int id, ComboBox::Style::Bits style)
			: ControlMaker ("COMBOBOX", winParent, id)
		{
			Style () << style
					 << ComboBox::Style::HasStrings 
					 << ComboBox::Style::AutoScroll 
					 << ComboBox::Style::NoIntegralHeight 
					 << Win::Style::AddVScrollBar;
		}
	};
}
#endif
