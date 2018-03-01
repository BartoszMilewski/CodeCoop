#if !defined (INSTANCE_H)
#define INSTANCE_H
//------------------------------------
//  (c) Reliable Software, 2002
//------------------------------------
#include <Win/GdiHandles.h>

namespace Win
{
	class Instance: public Win::Handle<HINSTANCE>
	{
	public:
		Instance (HINSTANCE h = 0) : Win::Handle<HINSTANCE> (h) {}
		operator HMODULE () const { return reinterpret_cast<HMODULE> (ToNative ()); }
		Icon::Handle LoadSharedIcon (int id, int width, int height, unsigned options);
	private:
		HANDLE LoadImg (int id, unsigned type, int width, int height, unsigned options);
	};
}
#endif
