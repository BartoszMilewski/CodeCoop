#if !defined (IMAGE_H)
#define IMAGE_H
//------------------------------------
//  (c) Reliable Software, 2002
//------------------------------------
#include "Resource/resource.h"

// 
// images used in listview
//
enum
{
    imageNew,
	imageDel,
	imageFile,
	imageFolder,
	imageDrive,
	imageDif,
    imageLast
};

enum
{
	overlayNone = 0,
	// one-based overlay indices
	overlayNew,
	overlayDel
};

int const IconId [imageLast] =
{
    ID_NEW,
	ID_DEL,
	ID_FILE,
	ID_FOLDER,
	ID_DRIVE,
	ID_DIF
};

#endif
