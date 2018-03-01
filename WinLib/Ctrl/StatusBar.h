#if !defined STATBAR_H
#define STATBAR_H
//--------------------------------
// (c) Reliable Software 1997-2000
//--------------------------------
#include <Ctrl/Controls.h>
#include <Graph/Font.h>
#include <Win/OwnerDraw.h>

namespace Win
{
	class StatusBar : public Win::ControlWithFont
	{
	public:
		class DrawHandler: public OwnerDraw::Handler
		{
		public:
			bool Draw (OwnerDraw::Item & item) throw ();
			virtual bool Draw (Win::Canvas canvas, Win::Rect const & rect, int itemId) = 0;
		};
	public:
		class Style: public Win::Style
		{
		public:
			enum Bits
			{
				Bottom = CCS_BOTTOM,
				SizeGrip = SBARS_SIZEGRIP
			};
		};

	public:
		StatusBar (Win::Dow::Handle winParent, int id)
			: Win::ControlWithFont (winParent, id),
			  _height (0)
		{}
		StatusBar (Win::Dow::Handle win = 0)
			: Win::ControlWithFont (win),
			  _height (0)
		{}
		void AddPart (int sizePerCent)
		{
			_partsPerCent.push_back (sizePerCent);
			_parts.push_back (0);
			if (_height == 0)
			{
				SetStatusBarHeight ();
			}
		}

		void SetFont (Font::Handle font)
		{
			Win::ControlWithFont::SetFont (font);
			SetStatusBarHeight ();
		}

		void SetFont (Font::Descriptor const & newFont)
		{
			Win::ControlWithFont::SetFont (newFont);
			SetStatusBarHeight ();
		}

		void ReSize (int left, int top, int width, int height);

		void SetText (char const * text, unsigned int idx = 0, bool sunken = true)
		{
			Assert (idx < _parts.size ());
			unsigned type = sunken? 0: SBT_POPOUT;
			SendMsg (SB_SETTEXT, idx | type, reinterpret_cast<LPARAM>(text));
		}

		void SetOwnerDraw (StatusBar::DrawHandler * handler, unsigned int idx = 0, void * data = 0)
		{
			RegisterOwnerDraw (handler);
			SendMsg (SB_SETTEXT, idx | SBT_OWNERDRAW, reinterpret_cast<LPARAM> (data));
		}

		int GetTextLen (unsigned int idx = 0) const
		{
			Assert (idx < _parts.size ());
			int retVal = SendMsg (SB_GETTEXTLENGTH, idx);
			return retVal & 0xffff;
		}

		std::string StatusBar::GetString (int idx = 0) const;
		bool GetPartRect (Win::Rect & partRect, int idx = 0)
		{
			return SendMsg (SB_GETRECT, idx, reinterpret_cast<LPARAM>(&partRect)) != 0;
		}

		int Height () const { return _height; }

	private:
		void GetText (char * buf, int idx = 0) const
		{
			SendMsg (SB_GETTEXT, idx, reinterpret_cast<LPARAM>(buf));
		}

		void SetStatusBarHeight ()
		{
			Win::Rect partRect;
			if (GetPartRect (partRect) != 0)
			{
				_height = partRect.bottom - partRect.top;
			}
		}

	private:
		int					_height;
		std::vector<int>	_partsPerCent;
		std::vector<int>	_parts;
	};

	inline Win::Style & operator<< (Win::Style & style, Win::StatusBar::Style::Bits bits)
	{
		style.OrIn (static_cast<Win::Style::Bits> (bits));
		return style;
	}

	class StatusBarMaker : public ControlMaker
	{
	public:
		StatusBarMaker (Win::Dow::Handle winParent, int id);
		StatusBar::Style & Style () { return static_cast<StatusBar::Style &> (_style); }
		Win::Dow::Handle Create ();
	};

	inline bool StatusBar::DrawHandler::Draw (OwnerDraw::Item & item)
	{
		return Draw (item.Canvas (), item.Rect (), item.ItemId ());
	}
}

#endif
