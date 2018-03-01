#if !defined (TOOLBAR_H)
#define TOOLBAR_H
//----------------------------------
// (c) Reliable Software 1998 - 2006
//----------------------------------

#include <Ctrl/Tooltip.h>
#include <Ctrl/Command.h>
#include <Ctrl/Controls.h>
#include <Graph/ImageList.h>
#include <Win/ControlHandler.h>

namespace Tool
{
	// Note: button width is the width of each button image in the button strip bitmap
	int const DefaultButtonWidth = 18;
	int const DefaultButtonHeight = 16;
	int const DefaultBarHeight = DefaultButtonHeight + 10;// Heuristics:
														  // add 10 pixels to the button height,
														  // so controls like drop down fit nicely

	class Button;

	class Handle : public Win::SimpleControl
	{
	public:
		Handle (Win::Dow::Handle win = 0)
			: Win::SimpleControl (win)
		{}

		int ButtonCount () const
		{
			return SendMsg (TB_BUTTONCOUNT);
		}
		bool GetMaxSize (unsigned & width, unsigned & height) const;
		// Windows default is 24x22 pixels
		void SetButtonSize (int width, int height)
		{
			SendMsg (TB_SETBUTTONSIZE, 0, MAKELONG(width, height));
		}
		void SetImageList (ImageList::Handle imageList)
		{
			SendMsg (TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(imageList.ToNative ()));
		}
		void AddButtons (std::vector<Button> const & buttons)
		{
			if (buttons.size () != 0)
				SendMsg (TB_ADDBUTTONS, buttons.size (), reinterpret_cast<LPARAM>(&buttons [0]));
		}
		void AutoSize ()
		{
			SendMsg (TB_AUTOSIZE);
		}
		int  CmdIdToButtonIndex (int cmdId);
		void GetButtonRect (int index, Win::Rect & rect) const;
		void ClearButtons ();
		void AddWindow (Win::Dow::Handle overlayWin, Win::Dow::Handle toolTipHandlerWin);
		unsigned  GetToolTipDelay () const;
		void SetToolTipDelay (unsigned miliSeconds) const;
		// Button manipulation
		bool Delete (int idx)
		{
			return SendMsg (TB_DELETEBUTTON, idx) != FALSE;
		}
		void EnableButton (int cmdID)
		{
			SendMsg (TB_ENABLEBUTTON, (WPARAM) cmdID, MAKELPARAM (TRUE, 0));
		}

		void DisableButton (int cmdID)
		{
			SendMsg (TB_ENABLEBUTTON, (WPARAM) cmdID, MAKELPARAM (FALSE, 0));
		}

		void HideButton (int cmdID)
		{
			SendMsg (TB_HIDEBUTTON, (WPARAM) cmdID, MAKELPARAM (TRUE, 0));
		}

		void ShowButton (int cmdID)
		{
			SendMsg (TB_HIDEBUTTON, (WPARAM) cmdID, MAKELPARAM (FALSE, 0));
		}

		void PressButton (int cmdID)
		{
			SendMsg (TB_PRESSBUTTON, (WPARAM) cmdID, MAKELPARAM (TRUE, 0));
		}

		void ReleaseButton (int cmdID)
		{
			SendMsg (TB_PRESSBUTTON, (WPARAM) cmdID, MAKELPARAM (FALSE, 0));
		}
		
		void InsertSeparator (int idx, int width);
	};

	class Style: public Win::Style
	{
	public:
		enum Bits
		{
		    Tips = TBSTYLE_TOOLTIPS,
		    AlignBottom = CCS_BOTTOM,
		    Flat = TBSTYLE_FLAT,
		    Transparent = TBSTYLE_TRANSPARENT,
		    NoMoveY = CCS_NOMOVEY, // don't move vertically when auto sizing
			NoResize = CCS_NORESIZE,
			NoParentAlign = CCS_NOPARENTALIGN,
			Wrapable = TBSTYLE_WRAPABLE,
			NoDivider = CCS_NODIVIDER
		};
	};

	inline Win::Style & operator<<(Win::Style & style, Tool::Style::Bits bits)
	{
		style.OrIn (static_cast<Win::Style::Bits> (bits));
		return style;
	}

	class Maker : public Win::ControlMaker
	{
	public:
		Maker (Win::Dow::Handle winParent, int id)
			: ControlMaker(TOOLBARCLASSNAME, winParent, id)
		{
			Win::CommonControlsRegistry::Instance()->Add(Win::CommonControlsRegistry::BAR);
		}

		Tool::Handle Create () 
		{
			Tool::Handle h = ControlMaker::Create ();
			h.SendMsg (TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON));
			return h;
		}
	};

	class Item
	{
	public:
		enum { idSeparator = -1, idEnd = -2 };
	public:
		int buttonId;           // Zero based button id (left-most = 0)
		char const * cmdName;   // Command
		char const * tip;       // Tool tip
	};

	// Tool::Bar with tool tips and variable layout
	// Button availability checked using cmdVector
	// at every refresh.
	class Bar : public Tool::Handle
	{
	public:
		Bar (Win::Dow::Handle winParent,
			 int toolbarId,
			 int bitmapId,
			 int buttonWidth,
			 Cmd::Vector const & cmdVector,
			 Tool::Item const * buttonItems,
			 Win::Style const & barStyle = Win::Style (Tool::Style::NoMoveY | Tool::Style::Tips));

		// Call this when you want to display all buttons
		void SetAllButtons (); // Does not refresh

		// Call this when you want to display a subset of buttons
		// layout is an array of button ids (including separators)
		// terminated with Item::idEnd
		void SetLayout (int const * layout); // Does not refresh

		// Tests the availability of commands and updates the state of each button
		void Refresh ();

		// In pixels
		unsigned GetButtonsEndX () const;
		unsigned GetButtonWidth (int buttonIndex) const;

		// Disable all buttons
		void Disable () throw ();
		void FillToolTip (Tool::TipForCtrl * tip) const;
		bool IsCmdButton (int cmdId) const;		 

		typedef std::vector<int>::const_iterator ButtonIdIter;
		ButtonIdIter begin () const { return _curCmdIds.begin (); }
		ButtonIdIter end () const { return _curCmdIds.end (); }

	protected:
		ImageList::AutoHandle	_buttonImages;
		Cmd::Vector const &		_cmdVector;
		Tool::Item const *		_buttonItems;	// Descriptions of all buttons that can appear on the tool bar
		std::map<int, int>		_buttonId2CmdId;
		std::map<int, int>		_cmdId2ButtonIdx;
		std::vector<int>		_curCmdIds;		// Cmd ids of the current button set placed on the tool bar
	};

	class Button: public TBBUTTON
	{
	public:
		Button ();
	};

	class BarSeparator : public Button
	{
	public:
		BarSeparator (int width = 0);
	};

	class BarButton : public Button
	{
	public:
		BarButton (int buttonId, int cmdId);
	};

	class ButtonCtrl : public Control::Handler
	{
	public:
		ButtonCtrl (int id, Cmd::Executor & executor)
			: Control::Handler (id),
			_executor (executor)
		{}

		bool OnControl (unsigned id, unsigned notifyCode) throw (Win::Exception);

		void ClearButtonIds () { _myButtonIds.clear (); }
		void AddButtonIds (Tool::Bar::ButtonIdIter first, Tool::Bar::ButtonIdIter last);
		bool IsCmdButton (int id) const;

	private:
		Cmd::Executor &		_executor;
		std::vector<int>	_myButtonIds;
	};
}

#endif
