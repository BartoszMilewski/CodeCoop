#if !defined (COLOR_H)
#define COLOR_H
// ----------------------------------
// (c) Reliable Software, 2000 - 2006
// ----------------------------------

namespace Win
{
	class Color
	{
	public:
		Color (COLORREF color = 0) : _color (color) {}
		Color (int r, int g, int b)
			: _color (RGB (r, g, b))
		{}
		void Init (int r, int g, int b)
		{
			_color = RGB (r, g, b);
		}

		COLORREF ToNative () const { return _color; }
		static Color Window () { return Color (::GetSysColor (COLOR_WINDOW)); }
		static Color ToolTipBkg () { return Color (::GetSysColor (COLOR_INFOBK)); }

		// Note: Windows GetRValue, etc, macros cause runtime warnings
		int R () const { return _color & 0xff; }
		int G () const { return (_color >> 8) & 0xff; }
		int B () const { return (_color >> 16) & 0xff; }

	private:
		COLORREF _color;
	};
	
	namespace Color3d
	{
		static Color Face () { return Color (::GetSysColor (COLOR_3DFACE)); }
		static Color Hilight () { return Color (::GetSysColor (COLOR_3DHILIGHT)); }
		static Color Light () { return Color (::GetSysColor (COLOR_3DLIGHT)); }
		static Color Shadow () { return Color (::GetSysColor (COLOR_3DSHADOW)); }
	};

	namespace ColorText
	{
		static Color Greyed () { return Color (::GetSysColor (COLOR_GRAYTEXT)); }
		static Color Hilight () { return Color (::GetSysColor (COLOR_HIGHLIGHTTEXT)); }
		static Color Red () { return Color (200, 0, 0); }
	};
}

#endif
