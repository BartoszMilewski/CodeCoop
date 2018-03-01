#if !defined (REBAR_H)
#define REBAR_H
//------------------------------------
//  (c) Reliable Software, 2005 - 2006
//------------------------------------

#include <Ctrl/Controls.h>
#include <Ctrl/Toolbar.h>
#include <climits>

namespace Tool
{
    static unsigned int const InvalidBandId = UINT_MAX;

	struct BandItem
	{
		unsigned int	bandId;
		int const *		buttonLayout;
		unsigned int	extraSpace;	// In pixels
	};

	class Rebar : public Win::SimpleControl
	{
	public:
		class Style: public Win::Style
		{
		public:
			enum Bits
			{
				AutoSize = RBS_AUTOSIZE,				// The rebar control will automatically change
														// the layout of the bands when the size or
														// position of the control changes.
														// An RBN_AUTOSIZE notification will be sent when this occurs.
				BandBorders = RBS_BANDBORDERS,			// The rebar control displays narrow lines to separate adjacent bands.
				DoubleClickToggle = RBS_DBLCLKTOGGLE,	// The rebar band will toggle its maximized or minimized
														// state when the user double-clicks the band.
														// Without this style, the maximized or minimized state
														// is toggled when the user single-clicks on the band.
				FixedOrder = RBS_FIXEDORDER,			// The rebar control always displays bands in the same order.
														// You can move bands to different rows, but the band order is static.
				RegisterDrop = RBS_REGISTERDROP,		// The rebar control generates RBN_GETOBJECT notification messages
														// when an object is dragged over a band in the control.
														// To receive the RBN_GETOBJECT notifications, initialize OLE with a call
														// to OleInitialize or CoInitialize.
				ToolTips = RBS_TOOLTIPS, 
				VarHeight = RBS_VARHEIGHT,				// The rebar control displays bands at the minimum required height,
														// when possible. Without this style, the rebar control displays all
														// bands at the same height, using the height of the tallest visible
														// band to determine the height of other bands.
				VerticalGripper = RBS_VERTICALGRIPPER	// The size grip will be displayed vertically instead of horizontally
														// in a vertical rebar control. This style is ignored for rebar controls
														// that do not have the CCS_VERT style.
			};
		};

		class BandInfo : public REBARBANDINFO
		{
		public:
			BandInfo ();

			unsigned GetBandWidth () const { return cx; }
			void SetCaption (std::string const & caption);
			void AddChildWin (Win::Dow::Handle win);
			void SetId (unsigned id);
			void InitBandSizes (unsigned int width, unsigned int height);

		private:
			std::string	_caption;
		};

	public:
		Rebar (Win::Dow::Handle win = 0)
			: Win::SimpleControl (win)
		{}

		void Size (Win::Rect const & toolRect);
		void AppendBand (Tool::Rebar::BandInfo const & info);
		void ShowBand (unsigned int bandIdx, bool show);
		bool DeleteBand (unsigned int bandIdx);
		void Clear ();
	};

	class RebarMaker: public Win::ControlMaker
	{
	public:
		RebarMaker (Win::Dow::Handle parentWin, int id);

		Win::Dow::Handle Create ()
		{
			return ControlMaker::Create ();
		}
	};

	class ButtonBand : public Tool::Rebar::BandInfo
	{
	public:
		ButtonBand (Win::Dow::Handle parentWin,
					unsigned buttonsBitmapId,
					unsigned buttonWidth,
					Cmd::Vector const & cmdVector,
					Tool::Item const * buttonItems,
					Tool::BandItem const * bandItem);

		unsigned GetId () const { return _id; }
		Win::Dow::Handle GetWindow () const { return _toolBar; }
		unsigned GetButtonExtent () const;
		void RegisterToolTip (Win::Dow::Handle registeredWin,
							  Win::Dow::Handle winNotify,
							  std::string const & tip);
		bool FillToolTip (Tool::TipForCtrl * tip) const;
		bool FillToolTip (Tool::TipForWindow * tip) const;
		void Disable () { _toolBar.Disable (); }
		void Enable () { _toolBar.Enable (); }
		void RefreshButtons () { _toolBar.Refresh (); }

		Tool::Bar::ButtonIdIter begin () const { return _toolBar.begin (); }
		Tool::Bar::ButtonIdIter end () const { return _toolBar.end (); }

	private:
		void CalculateToolTipDelay (char const * tip) const;

	private:
		class HWinLess : public std::binary_function<Win::Dow::Handle, Win::Dow::Handle, bool>
		{
		public:
			bool operator () (Win::Dow::Handle win1, Win::Dow::Handle win2) const
			{
				return win1.ToNative () < win2.ToNative ();
			}
		};

		typedef std::map<Win::Dow::Handle, std::string, HWinLess> ToolTipMap;

	private:
		unsigned	_id;
		ToolTipMap	_registeredToolTips;
		unsigned	_defaultToolTipDelay;

	protected:
		Tool::Bar	_toolBar;
	};
}

namespace Notify
{
	// Subclass RebarHandler overwriting some of its methods.
	// In your controller, overwrite the following method to return your handler
	// Notify::Handler * Win::Controller::GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom)
	class RebarHandler : public Notify::Handler
	{
	public:
		explicit RebarHandler (unsigned id)
			: Notify::Handler (id)
		{}

		virtual bool OnAutoSize (Win::Rect const & targetRect,
								 Win::Rect const & actualRect,
								 bool changeDetected) throw () { return false; }
		virtual bool OnChevronPushed (unsigned bandIdx,
									  unsigned bandId,
									  unsigned appParam,
									  Win::Rect const & chevronRect,
									  unsigned notificationParam) throw () { return false; }
		virtual bool OnChildSize (unsigned bandIdx,
								  unsigned bandId,
								  Win::Rect & newChildRect,
								  Win::Rect const & newBandRect) throw () { return false; }
		virtual bool OnHeightChange () throw () { return false; }
		virtual bool OnLayoutChange () throw () { return false; }

	protected:
		bool OnNotify (NMHDR * hdr, long & result);
	};
}

#endif
