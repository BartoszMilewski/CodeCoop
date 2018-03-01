//---------------------------
// (c) Reliable Software 2006
//---------------------------
#include "precompiled.h"
#include "WikiDef.h"

char const Wiki::SYSTEM [] = "system";
char const Wiki::SYSTEM_PREFIX [] = "system:";
unsigned Wiki::SYSTEM_PREFIX_SIZE = 7;

char const * Wiki::ImageOpenPattern = 
	"Image File (*.jpg, *.gif, *.png)\0*.jpg;*.gif;*.png\0All Files (*.*)\0*.*\0";

char const * const Wiki::ImagePattern [] = 
{
	"*.jpg",
	"*.gif",
	"*.png",
	0
};
