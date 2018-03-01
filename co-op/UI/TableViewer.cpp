/-----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "precompiled.h"
#include "TableViewer.h"
#include "Image.h"

#include <Graph/Icon.h>
#include <Graph/ImageList.h>

TableViewer::TableViewer (Win::Dow::Handle winParent, int id, bool isSingleSelection)
	: _images (16, 16, imageLast), _columnImages (16, 16, 2)
{
	Win::Instance inst = winParent.GetInstance ();
    for (int i = 0; i < imageLast; i++)
    {
		Icon::SharedMaker icon (16, 16);
		_images.AddIcon (icon.Load (inst, IconId [i]));
    }
    // Set overlay indicies
    _images.SetOverlayImage (imageOverlaySynchNew, overlaySynchNew);
    _images.SetOverlayImage (imageOverlaySynchDelete, overlaySynchDelete);
    _images.SetOverlayImage (imageOverlaySynch, overlaySynch);
    _images.SetOverlayImage (imageOverlaySynchProj, overlaySynchProj);
	_images.SetOverlayImage (imageOverlayFullSyncProj, overlayFullSyncProj);
	_images.SetOverlayImage (imageOverlayRecoveryProj, overlayRecoveryProj);
    _images.SetOverlayImage (imageOverlayLocked, overlayLocked);

	Win::ReportMaker reportMaker (winParent, id);
	reportMaker.Style () << Win::ListView::Style::EditLabels;
	if (isSingleSelection)
		reportMaker.Style () << Win::ListView::Style::SingleSelection;
	Init (reportMaker.Create ());
    SetImageList (_images);
	// Images of sort arrows
	Icon::SharedMaker icon (16, 16);
	_columnImages.AddIcon (icon.Load (inst, I_SORTUP));
	_columnImages.AddIcon (icon.Load (inst, I_SORTDOWN));
	GetHeader ().SetImageList (_columnImages);
}

TableViewer::~TableViewer ()
{
	SetImageList ();
}


