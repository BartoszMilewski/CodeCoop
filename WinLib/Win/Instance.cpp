//------------------------------------
//  (c) Reliable Software, 2002
//------------------------------------
#include <WinLibBase.h>
#include <Win/Instance.h>
#include <Graph/Icon.h>

namespace Win
{
	Icon::Handle Instance::LoadSharedIcon (int id, int width, int height, unsigned options)
	{
		HICON hicon = reinterpret_cast<HICON> (
			LoadImg (id, IMAGE_ICON, width, height, options | LR_SHARED));
		if (hicon == 0)
			throw Win::Exception ("Icon Load failed");
		return Icon::Handle (hicon);
	}

	HANDLE Instance::LoadImg (int id, unsigned type, int width, int height, unsigned options)
	{
		return ::LoadImage (H (), MAKEINTRESOURCE (id), type, width, height, options);
	}
}
