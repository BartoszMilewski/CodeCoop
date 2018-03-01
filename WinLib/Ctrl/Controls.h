#if !defined CONTROLS_H
#define CONTROLS_H
//------------------------------------
//  (c) Reliable Software, 1996 - 2005
//------------------------------------

#include <Win/OwnerDraw.h>
#include <Win/WinMaker.h>

namespace Font { class Descriptor; }

namespace Win
{
	void UseCommonControls ();

	//
	//	Windows Common Controls (e.g. Header, ListView) need to be registered
	//	before they can be used.  The old API, InitCommonControls (which
	//	registered them all) has been superseded by InitCommonControlsEx,
	//	which allows client code to register only those classes that it will
	//	use.
	//
	//	class CommonControlsRegistry wraps the Windows InitCommonControlsEx
	//	API.  It is a singleton (there is only one instance of the class),
	//	accessed through the static Instance method.  For example:
	//
	//		CommonControlsRegistry::Instance()->Add(CommonControlsRegistry::LISTVIEW);
	//
	//	Other Common Controls can be add the the CommonControlsRegistry enum as
	//	needed.
	//

	class CommonControlsRegistry
	{
	public:
		enum Type
		{
			LISTVIEW = ICC_LISTVIEW_CLASSES, // listview, header
			TREEVIEW = ICC_TREEVIEW_CLASSES, // treeview, tooltips
			BAR = ICC_BAR_CLASSES, // toolbar, statusbar, trackbar, tooltips
			TAB = ICC_TAB_CLASSES, // tab, tooltips
			UPDOWN = ICC_UPDOWN_CLASS, // updown
			PROGRESS = ICC_PROGRESS_CLASS, // progress
			HOTKEY = ICC_HOTKEY_CLASS, // hotkey
			ANIMATE = ICC_ANIMATE_CLASS, // animate
			WIN95 = ICC_WIN95_CLASSES,
			DATE = ICC_DATE_CLASSES, // month picker, date picker, time picker, updown
			COMBOEX = ICC_USEREX_CLASSES, // ComboBoxEx
			COOL = ICC_COOL_CLASSES // rebar (coolbar) control
		};

		void Add(Type types);

		static CommonControlsRegistry *Instance()
		{
			return &_ccreg;
		}

	private:
		INITCOMMONCONTROLSEX _icc;
		static CommonControlsRegistry _ccreg;

		CommonControlsRegistry()
		{
			_icc.dwSize = sizeof(_icc);
			_icc.dwICC = 0;
		}

	};

	class SimpleControl : public Win::Dow::Handle
	{
	public:
		SimpleControl (Win::Dow::Handle winParent, int id)
			: Win::Dow::Handle (::GetDlgItem (winParent.ToNative (), id))
		{
			Assert (winParent == GetParent ());
			Assert (id == GetId ());
		}
		SimpleControl (Win::Dow::Handle win = 0)
			: Win::Dow::Handle (win)
		{}
		Win::Dow::Handle ToWin () const { return *this; }
		void Reset (Win::Dow::Handle h) { Win::Dow::Handle::Reset (h.ToNative ()); }
		void Init (Win::Dow::Handle winParent, int id)
		{
			Reset (::GetDlgItem (winParent.ToNative (), id));
			Assert (!IsNull ());
			Assert (winParent == GetParent ());
			Assert (id == GetId ());
		}
		void RegisterOwnerDraw (OwnerDraw::Handler * handler);
		void UnregisterOwnerDraw ();

		// code is the HIWORD (wParam)
		static bool IsClicked (int code)
		{
			return code == BN_CLICKED;
		}
	};

	class ControlWithFont: public Win::SimpleControl
	{
	public:
		ControlWithFont (Win::Dow::Handle winParent, int id)
			: Win::SimpleControl (winParent, id)
		{}
		ControlWithFont (Win::Dow::Handle win = 0)
			: Win::SimpleControl (win)
		{}
		Win::Dow::Handle ToWin () const { return Win::SimpleControl::ToWin (); }
		void SetFont (int pointSize, std::string const & face);
		void SetFont (Font::Descriptor const & newFont);
		//	make visible the base class SetFont that would
		//	otherwise be hidden by above SetFont
		using SimpleControl::SetFont;	// SetFont(Font::Handle font);
	private:
		ControlWithFont (ControlWithFont const &);
		ControlWithFont & operator= (ControlWithFont const &);
	private:
		Font::AutoHandle	_font;
	};

	class ControlMaker: public Win::ChildMaker
	{
	public:
		//	No edge--Use AddClientEdge() if needed
		ControlMaker (char const * className, Win::Dow::Handle winParent, int id)
			: Win::ChildMaker (className, winParent, id)
		{
			_style << Win::Style::Visible;
		}
		void RegisterOwnerDraw (OwnerDraw::Handler * handler);
		// Notice: controls are children of other windows and are automatically
		// destroyed when their parents are destroyed
		using Win::ChildMaker::Create;

		// parent receives child messages, canvas is where the child paints itself
		Win::Dow::Handle Create (Win::Dow::Handle canvas)
		{
			Win::Dow::Handle h = ChildMaker::Create ();
			h.SetParent (canvas.ToNative ());
			return h;
		}
	};
}

#endif
