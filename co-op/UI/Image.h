#if !defined (IMAGE_H)
#define IMAGE_H
//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "resource.h"

// 
// images used in listview
//
enum
{
    imageOverlaySynchNew,
    imageOverlaySynchDelete,
    imageOverlaySynch,
    imageOverlaySynchProj,
	imageOverlayFullSyncProj,
	imageOverlayRecoveryProj,
	imageOverlayLocked,
    imageCurOutProj,
    imageOutProj,
    imageFolder,
	imageFolderIn,
    imageNone,
	imageWait,
    imageCheckedIn,
    imageCheckedOut,
    imageNewFile,
    imageDeleted,
	imageMerge,
	imageMergeConflict,
    imageScript,
    imageOutgoingScript,
	// Script Icons
	imageVersion1,
	imageVersion2,
	imageVersionRange1,
	imageVersionRange2,
	imageVersionCurrent,
    imageNextScript,
    imageIncomingScript,
	imageAwaitingScript,
	imageMissingScript,
	imageRejectedScript,
	// Reverse Script Icons
	imageVersion1_r,
	imageVersion2_r,
	imageVersionRange1_r,
	imageVersionRange2_r,
	imageVersionCurrent_r,
    imageNextScript_r,
    imageIncomingScript_r,
	imageAwaitingScript_r,
	imageMissingScript_r,
	imageRejectedScript_r,

	imageLabel,
	imageProject,
	imageCurProject,
	imageCoop,
	imageSortUp,
	imageSortDown,
    imageLast
};

// Offset to be added to script icon ID to reverse the image
int const VersionIconCount = 10;

enum
{
	overlayNone = 0,
	// one-based overlay indexes
	overlaySynchNew,
	overlaySynchDelete,
	overlaySynch,
	overlaySynchProj,
	overlayFullSyncProj,
	overlayRecoveryProj,
	overlayLocked
};

int const IconId [imageLast] =
{
    I_SYN_NEW,
    I_SYN_DEL,
    I_SYN,
	I_SYN_PROJ,
	I_FULL_SYN,
	I_RECOVERY_PROJ,
	I_LOCKED,
	I_CUR_OUT_PROJ,
	I_OUT_PROJ,
    I_FOLDER,
	I_FOLDER_IN,
    I_NONE,
	I_WAIT,
    I_IN,
    I_OUT,
    I_NEW,
    I_DEL,
	I_MERGE,
	I_MERGE_CONFLICT,
	I_SCRIPT,
    I_OUT_SCRIPT,
	// Script Icons
	I_VERSION,
	I_VERSION2,
	I_VRANGE1,
	I_VRANGE2,
	I_VERSION_CUR,
    I_NEXT_SCRIPT,
    I_IN_SCRIPT,
	I_AWAITING,
	I_MISSING_SCRIPT,
	I_REJECTED_SCRIPT,
	// Reverse Script Icons
	I_VERSION_R,
	I_VERSION2_R,
	I_VRANGE1_R,
	I_VRANGE2_R,
	I_VERSION_CUR_R,
    I_NEXT_SCRIPT,
    I_IN_SCRIPT,
	I_AWAITING,
	I_MISSING_SCRIPT,
	I_REJECTED_SCRIPT,

	I_LABEL,
	I_PROJECT,
	I_CURPROJ,
	ID_COOP,
	I_SORTUP,
	I_SORTDOWN
};

#endif
