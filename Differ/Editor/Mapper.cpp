//
// (c) Reliable Software, 1997
//

#include "precompiled.h"
#include "Mapper.h"
#include "Lines.h"
#include <Graph/Font.h>
#include <Graph/Canvas.h>

ColMapper::ColMapper (Win::Dow::Handle hwnd, Font::Handle font)
{
	Win::UpdateCanvas canvas (hwnd);
	Font::Holder fontHolder (canvas, font);
	canvas.GetCharSize (_cx, _cy);
}

int Tabber::ExpandTabs (EditParagraph const * para, int start, int len, int startCol) const
{
	int expandCol = startCol;
	for (int i = 0; i < len; i++)
	{
		expandCol++;
		if ((*para) [start + i] == '\t')
			expandCol = SkipTab (expandCol);
	}
	return expandCol;
}
