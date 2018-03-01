#if !defined (PEN_H)
#define PEN_H
// -----------------------------
// (c) Reliable Software, 2000
// -----------------------------

#include <Win/GdiHandles.h>
#include <Graph/StockObj.h>
#include <Graph/Color.h>
#include <Graph/Canvas.h>

namespace Pen
{
	enum Style
	{
		Null  = PS_NULL,
		Solid = PS_SOLID,
		Dash  = PS_DASH
	};

	class Maker
	{
	public:
		Maker (Pen::Style style = Pen::Solid)
		{
			_logicalPen.lopnStyle = style;
			_logicalPen.lopnWidth.x = 0; // minimal width
			_logicalPen.lopnColor = Win::Color ().ToNative (); // black
		}
		void SetWidth (int width) 
		{
			_logicalPen.lopnWidth.x = width;
		}
		void SetColor (Win::Color color)
		{
			_logicalPen.lopnColor = color.ToNative ();
		}
		AutoHandle Create ()
		{
			HPEN h = ::CreatePenIndirect (&_logicalPen);
			if (h == Handle::NullValue ())
				throw Win::Exception ("Cannot create pen indirect");
			return AutoHandle (h);
		}
	private:
		LOGPEN _logicalPen;
	};

	// Revisit: encapsulate LOGPEN, use GetObject to initialize it (same for EXTLOGPEN)
	
	class Color: public AutoHandle
	{
	public:
		Color () {}
		explicit Color (Win::Color color, int width = 0, Pen::Style style = Pen::Solid)
			: AutoHandle (::CreatePen (style, width, color.ToNative ()))
		{}
	};

	typedef Gdi::ObjectHolder<Pen::Handle> Holder;

	class InstantHolder
	{
	public:
		InstantHolder (Win::Canvas canvas, Win::Color color)
			: _pen (color),
			  _holder (canvas, _pen)
		{}
	private:
		Pen::Color	_pen;
		Pen::Holder	_holder;
	};

	class Pens3d
	{
	public:
		Pens3d ();
		Color & Hilight () { return _penHilight; }
		Color & Light () { return _penLight; }
		Color & Shadow () { return _penShadow; }
		Color & DkShadow () { return _penDkShadow; }
	private:
		Color	_penHilight;
		Color	_penLight;
		Color	_penShadow;
		Color	_penDkShadow;
	};

	inline Pens3d::Pens3d ()
	:
		_penLight (GetSysColor (COLOR_3DLIGHT)),
		_penHilight (GetSysColor (COLOR_3DHILIGHT)),
		_penShadow (GetSysColor (COLOR_3DSHADOW)),
		_penDkShadow (GetSysColor (COLOR_3DDKSHADOW))
	{}

	enum StockType
	{
		StockWhite = WHITE_PEN,
		StockBlack = BLACK_PEN,
		StockNull = NULL_PEN,
	};

	typedef Stock::ObjectHolder<Pen::Holder, Pen::StockType> StockHolder;

	// specialized classes
	class White : public StockHolder
	{
	public:
		White (Win::Canvas canvas): StockHolder (canvas, Pen::StockWhite) {}
	};

	class Black : public StockHolder
	{
	public:
		Black (Win::Canvas canvas): StockHolder (canvas, Pen::StockBlack) {}
	};

	#if 0 // useful for debugging
	inline Pens3d::Pens3d ()
	:
		_penLight (RGB (128, 64, 0)),
		_penHilight (RGB (255, 128, 0)),
		_penShadow (RGB (0, 0, 256)),
		_penDkShadow (RGB (0, 0, 128))
	{}
	#endif
}

#endif
