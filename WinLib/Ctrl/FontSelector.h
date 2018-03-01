#if !defined (FONTSELECTOR_H)
#define FONTSELECTOR_H
//
// (c) Reliable Software 1997, 98, 99, 2000
//

#include <Graph/Font.h>

namespace Font
{
	class Selector
	{
	public:
		Selector ();

		void SetNoFaceSelection ()   { _fontSelector.Flags |= CF_NOFACESEL; }
		void SetNoStyleSelection ()  { _fontSelector.Flags |= CF_NOSTYLESEL; }
		void SetNoSizeSelection ()   { _fontSelector.Flags |= CF_NOSIZESEL; }
		void SetNoScriptSelection () { _fontSelector.Flags |= CF_NOSCRIPTSEL; }
		void SetFixedPitchOnly ()    { _fontSelector.Flags |= CF_FIXEDPITCHONLY; }
		void SetTrueTypeOnly ()      { _fontSelector.Flags |= CF_TTONLY; }
		void SetScreenFontsOnly ()   { _fontSelector.Flags |= CF_SCREENFONTS;
										_fontSelector.nFontType = SCREEN_FONTTYPE; }
		void SetUseLogFont ()        { _fontSelector.Flags |= CF_INITTOLOGFONTSTRUCT; }
		
		void LimitSizeSelection (int min, int max);
		void SetDefaultSelection (char const * faceName, int pointSize);
		void SetDefaultSelection (Maker const & defaultFont);

		bool Select ();
		// Wiesiek: Revisit: Why is Maker passed around? See usage of Font::Selector
		Maker const & GetSelection () const { return _fontMaker; }
		Maker & GetFontMaker () { return _fontMaker; }

	private:
		static int CALLBACK EnumFontFamCharSet (LOGFONT const * logFont,		// pointer to logical-font data
												TEXTMETRIC const * fontMetrics,	// pointer to physical-font data
												ULONG type,						// type of font
												LPARAM lParam);					// application-defined data 
		void MatchCharSet ();

	private:
		struct keyboardInfo
		{
			HKL		layout;
			BYTE	charSet;
		};

		Maker			_fontMaker;
		CHOOSEFONT		_fontSelector;
		keyboardInfo	_keyboard;
	};
}
#endif
