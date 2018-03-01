#if !defined (CANVTOOLS_H)
#define CANVTOOLS_H
// -----------------------------
// (c) Reliable Software, 2000
// -----------------------------
#include "Canvas.h"

namespace Win
{
	// Drawing modes

	class ForegrMode
	{
	public:
		ForegrMode (Win::Canvas canvas, int mode = R2_COPYPEN)
		: _canvas (canvas)
		{
			_modeOld = ::GetROP2 (canvas.ToNative ());
			::SetROP2 (canvas.ToNative (), mode);
		}
		~ForegrMode ()
		{
			::SetROP2 (_canvas.ToNative (), _modeOld);
		}
	private:
		Win::Canvas _canvas;
		int _modeOld;
	};

	class BkgrMode
	{
	public:
		BkgrMode (Win::Canvas canvas, int mode)
			: _canvas (canvas), _oldMode (::SetBkMode (canvas.ToNative (), mode))
		{}
		~BkgrMode ()
		{
			::SetBkMode (_canvas.ToNative (), _oldMode);
		}
	private:
		Win::Canvas _canvas;
		int     _oldMode;
	};
	// specialised classes
	class TransparentBkgr : public BkgrMode
	{
	public:
		TransparentBkgr (Win::Canvas canvas)
			: BkgrMode (canvas, TRANSPARENT)
		{}
	};
}

#endif
