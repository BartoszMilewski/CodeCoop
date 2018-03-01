#if !defined (BRUSH_H)
#define BRUSH_H
// -----------------------------
// (c) Reliable Software, 2000
// -----------------------------
#include <Win/GdiHandles.h>
#include <Graph/StockObj.h>
#include <Graph/Color.h>
#include <Graph/Bitmap.h>

namespace Brush
{
	enum Hatch
	{
		Vertical = HS_VERTICAL,
		Horizontal = HS_HORIZONTAL,
		Cross = HS_CROSS,
		DiagCross = HS_DIAGCROSS,
		BackDiagonal = HS_BDIAGONAL,
		FwdDiagonal = HS_FDIAGONAL
	};
	class AutoHandle : public Win::AutoHandle<Brush::Handle, Gdi::Disposal<Brush::Handle> >
	{
		typedef Win::AutoHandle<Brush::Handle, Gdi::Disposal<Brush::Handle> > Base;
	public:
		AutoHandle (Win::Color color)
			: Base (::CreateSolidBrush (color.ToNative ()))
		{}
		AutoHandle (Bitmap::Handle bmp)
			: Base (::CreatePatternBrush (bmp.ToNative ()))
		{}
		AutoHandle (Brush::Hatch hatch, Win::Color color)
			: Base (::CreateHatchBrush (hatch, color.ToNative ()))
		{}
	};

	// Revisit: encapsulate LOGBRUSH, use GetObject to initialize it

	typedef Gdi::ObjectHolder<Brush::Handle> Holder;

	class InstantHolder
	{
	public:
		InstantHolder (Win::Canvas canvas, Win::Color color)
			: _brush (color), 
				_holder (canvas, _brush)
		{}
	private:
		Brush::AutoHandle	_brush;
		Holder				_holder;
	};

	enum StockType
	{
		StockWhite = WHITE_BRUSH,
		StockBlack = BLACK_BRUSH,
		StockNull = NULL_BRUSH,
	};

	typedef Stock::ObjectHolder<Brush::Holder, Brush::StockType> StockHolder;

	// specialized classes
	class White : public StockHolder
	{
	public:
		White (Win::Canvas canvas): StockHolder (canvas, Brush::StockWhite) {}
	};

	class Black : public StockHolder
	{
	public:
		Black (Win::Canvas canvas): StockHolder (canvas, Brush::StockBlack) {}
	};

	class Transparent: public StockHolder
	{
	public:
		Transparent (Win::Canvas canvas): StockHolder (canvas, Brush::StockNull) {}
	};
}

#endif
