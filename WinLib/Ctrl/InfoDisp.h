#if !defined INFODISP_H
#define INFODISP_H
//----------------------------------
// (c) Reliable Software 1997 - 2006
//----------------------------------

#include <Ctrl/Controls.h>
#include <Graph/Canvas.h>
#include <Ctrl/Edit.h>

namespace Win
{
	class InfoDisplay : public Win::ControlWithFont
	{
	public:
		class Style: public Win::Style
		{
		public:
			enum Bits
			{
				Sunken = SS_SUNKEN,
				Notify = SS_NOTIFY,
				LeftNoWordWrap = SS_LEFTNOWORDWRAP,
				NoPrefix = SS_NOPREFIX,
				PathEllipsis = SS_PATHELLIPSIS,
				WordEllipsis = SS_WORDELLIPSIS
			};
		};

	public:
		InfoDisplay (Win::Dow::Handle winParent, int id)
			: Win::ControlWithFont (winParent, id)
		{}
		InfoDisplay (Win::Dow::Handle win = 0)
			: Win::ControlWithFont (win)
		{}

		void ReSize (int left, int top, int width, int height)
		{
			Move (left, top, width, height);
		}

		void GetTextSize (char const * text, int & textLength, int & textHeight)
		{
			Win::UpdateCanvas canvas (H ());
			canvas.GetTextSize (text, textLength, textHeight);
		}
	};

	inline Win::Style & operator<< (Win::Style & style, Win::InfoDisplay::Style::Bits bits)
	{
		style.OrIn (static_cast<Win::Style::Bits> (bits));
		return style;
	}

	class InfoDisplayMaker : public Win::ControlMaker
	{
	public:
		InfoDisplayMaker (Win::Dow::Handle winParent, int id)
			: ControlMaker ("static", winParent, id)
		{
			Style () << InfoDisplay::Style::Notify
					 << InfoDisplay::Style::LeftNoWordWrap
					 << InfoDisplay::Style::NoPrefix
					 << InfoDisplay::Style::PathEllipsis
					 << InfoDisplay::Style::WordEllipsis
					 << Win::Style::Ex::WindowEdge;
		}
	};

	class ReadOnlyDisplay : public Win::ControlWithFont
	{
	public:
		// parent receives child messages, canvas is where the child paints itself
		ReadOnlyDisplay (int id, Win::Dow::Handle winParent, Win::Dow::Handle winCanvas)
			: Win::ControlWithFont (winParent)
		{
			Win::EditMaker displayMaker (winParent, id);
			displayMaker.Style () << Win::Edit::Style::ReadOnly
								  << Win::Edit::Style::AlignLeft
								  << Win::Style::Ex::WindowEdge;
			Reset (displayMaker.Create (winCanvas));
			Show ();
		}

		static bool GotFocus (int notifyCode) { return notifyCode == EN_SETFOCUS; }
		static bool LostFocus (int notifyCode) { return notifyCode == EN_KILLFOCUS; }
	};

}

#endif
