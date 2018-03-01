#if !defined (PAINTCONTEXT_H)
#define PAINTCONTEXT_H
// -------------------------------
// (c) Reliable Software 1997-2005
// -------------------------------

#include "Mapper.h"
#include "BrushBox.h"

#include <Graph/Color.h>
#include <Graph/Font.h>
#include <Graph/Canvas.h>
#include <Win/Win.h>

class EditParagraph;

class FontBox: public BrushBox
{
public:
	FontBox (Win::Dow::Handle win, bool useTabs, int tabSizeChar);
	Font::Handle & GetFont () { return _font; }
	bool GetUseTabs() const { return _useTabs; }
	int GetTabSizePix () { return _tabSizePix; }
	int GetTabSizeChar () const { return _tabSizeChar; }
	int	GetCharHeight () const { return _yChar; }
	int GetCharWidth () const { return _xChar; }
	void ChangeFont (Win::Dow::Handle win, unsigned tabSize, Font::Maker * newFont);

private:
	int			_xChar;
	int			_yChar;
	bool		_useTabs;
	int			_tabSizeChar;
	int			_tabSizePix;

	Font::AutoHandle	_font;
};

class PaintContext : public Win::PaintCanvas
{
public:
	PaintContext (Win::Dow::Handle win, FontBox & fontBox)
		: Win::PaintCanvas (win), 
		_fontBox (fontBox),
		_tabber (fontBox.GetTabSizeChar ()),
#pragma warning(disable: 4355) // 'this' used before initialized
		_fontHolder (*this, fontBox.GetFont ()) 
#pragma warning(default: 4355)
	{}

	int StyledTextOut (	int xOrg, 
						int x, 
						int y, 
						EditParagraph const * para, 
						int start, 
						int len); 

	int HighTextOut (	int xOrg, 
						int x, 
						int y, 
						EditParagraph const * para, 
						int start, 
						int len);

	int SimpleTextOut ( int xOrg, 
						int x, 
						int y, 
						EditParagraph const * para, 
						int start, 
						int len);

	void GetTextRect (	Win::Rect & rect, 
						int xOrg, 
						int x, 
						int y, 
						EditParagraph const * para, 
						int start, 
						int len);
private:
	Font::Holder _fontHolder;
	FontBox	  & _fontBox;
	Tabber		_tabber;
};

#endif
