#if !defined (FONT_H)
#define FONT_H
// ---------------------------------
// Reliable Software (c) 1998 - 2007
// ---------------------------------

#include <Graph/StockObj.h>
#include <Graph/Color.h>
#include <Graph/Canvas.h>

namespace Font
{
	class Weight
	{
	public:
		enum WeightBits
		{
			// Font weight is between 0 and 1000. These are the predefined values
			DontCare = FW_DONTCARE, 
			Thin = FW_THIN, 
			ExtraLight = FW_EXTRALIGHT, 
			Light = FW_LIGHT, 
			Normal = FW_NORMAL, 
			Medium = FW_MEDIUM, 
			SemiBold = FW_SEMIBOLD, 
			Bold = FW_BOLD, 
			ExtraBold = FW_EXTRABOLD, 
			Heavy = FW_HEAVY 
		};
	};

	class CharSet
	{
	public:
		enum CharSetBits
		{
			Ansi = ANSI_CHARSET,
			Baltic = BALTIC_CHARSET,
			ChineseBig5 = CHINESEBIG5_CHARSET,
			Default = DEFAULT_CHARSET,
			EastEurope = EASTEUROPE_CHARSET,
			Gb = GB2312_CHARSET,
			Greek = GREEK_CHARSET,
			Hangul = HANGUL_CHARSET,
			Mac = MAC_CHARSET,
			Oem = OEM_CHARSET,
			Russian = RUSSIAN_CHARSET,
			ShiftJIS = SHIFTJIS_CHARSET,
			Symbol = SYMBOL_CHARSET,
			Turkish = TURKISH_CHARSET,
			Vietnamese = VIETNAMESE_CHARSET, 
			//Korean language edition of Windows: 
			Johab = JOHAB_CHARSET, 
			// Middle East language edition of Windows: 
			Arabic = ARABIC_CHARSET,
			Hebrew = HEBREW_CHARSET, 
			//Thai language edition of Windows: 
			Thai = THAI_CHARSET 
		};
	};

	class Descriptor: public LOGFONT
	{
	public:
		Descriptor ()
		{
			SetDefault ();
		}
		Descriptor (Win::Canvas canvas);

		void SetDefault ();
		char const * GetFaceName () const { return &lfFaceName [0]; }
		int GetHeight () const { return lfHeight; }
		int GetWeight () const { return lfWeight; }
		bool IsItalic () const { return lfItalic != 0; }
		int GetCharSet () const { return lfCharSet; }
	};

	class Maker: public Descriptor
	{
	public:
		Maker ();
		Maker (int pointSize, std::string const & faceName);
		Maker (Font::Descriptor const & newFont);
		void Init (int pointSize, std::string const & faceName);
		int GetPointSize () const { return _pointSize; }
		void SetFaceName (std::string const & faceName);
		void SetPointSize (int pointSize);
		void ScaleUsing (Win::Canvas canvas);
		void MakeBold ()
		{
			lfWeight = Font::Weight::Bold;
		}
		void MakeHeavy ()
		{
			lfWeight = Font::Weight::Heavy;
		}
		void SetWeight (int weigth)
		{
			lfWeight = weigth;
		}
		void SetHeight (int height);
		void SetItalic (bool flag)
		{
			lfItalic = flag ? TRUE : FALSE;
		}
		void SetCharSet (int charSet)
		{
			lfCharSet = charSet;
		}

		Font::AutoHandle Create ();

	private:
		void UpdatePointSize ();
	private:
		int			_pointSize;
	};

	class Holder: public Gdi::ObjectHolder<Font::Handle>
	{
	public:
		Holder (Win::Canvas canvas, Font::Handle font)
			: Gdi::ObjectHolder<Font::Handle> (canvas, font)
		{}
		void GetAveCharSize(int &aveCharWidth, int &aveCharHeight);
		void GetBaseUnits (int &baseUnitX, int &baseUnitY);
		int  GetCharWidthTwips (char c, int pointSize);
	};

	class ColorHolder
	{
	public:
		ColorHolder (Win::Canvas canvas, Win::Color color)
			: _canvas (canvas), 
				_oldColor (::SetTextColor (_canvas.ToNative (), color.ToNative ()))
		{}
		~ColorHolder ()
		{
			::SetTextColor (_canvas.ToNative (), _oldColor);
		}
	private:
		Win::Canvas	_canvas;
		COLORREF	_oldColor;
	};

	class BkgHolder
	{
	public:
		BkgHolder (Win::Canvas canvas, Win::Color color)
			: _canvas (canvas), 
				_oldColor (::SetBkColor (_canvas.ToNative (), color.ToNative ()))
		{}
		~BkgHolder ()
		{
			::SetBkColor (_canvas.ToNative (), _oldColor);
		}
	private:
		Win::Canvas	_canvas;
		COLORREF	_oldColor;
	};

	enum StockType
	{
		StockSystem = SYSTEM_FONT,
		StockSystemFixed = SYSTEM_FIXED_FONT,
		StockProportional = ANSI_VAR_FONT,
		StockOemFixed = OEM_FIXED_FONT,
		StockDefaultGUI = DEFAULT_GUI_FONT
	};

	typedef Stock::Object<Font::Handle, Font::StockType> StockObject;

	class SysFixed: public StockObject
	{
	public:
		SysFixed () : StockObject (Font::StockSystemFixed) {}
	};
	
	class OemFixed: public StockObject
	{
	public:
		OemFixed () : StockObject (Font::StockOemFixed) {}
	};

	class GUIDefault : public StockObject
	{
	public:
		GUIDefault () : StockObject (Font::StockDefaultGUI) {}
	};
	
	typedef Stock::ObjectHolder<Font::Holder, Font::StockType> StockHolder;
}

#endif
