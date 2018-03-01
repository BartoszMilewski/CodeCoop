// ---------------------------
// (c) Reliable Software, 2001
// ---------------------------
#include "precompiled.h"
#include "View.h"

ItemView::ItemView (Win::Dow::Handle winParent, int id)
{
	Win::ReportMaker reportMaker (winParent, id);
	reportMaker.Style () << Win::ListView::Style::EditLabels
						<< Win::Style::Ex::ClientEdge;
	Init (reportMaker.Create ());
}
