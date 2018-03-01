#if !defined CONTROLS_H
#define CONTROLS_H
//-----------------------------------------------
//  controls.h
//  Window Controls
//  (c) Reliable Software, 1996-2001
//-----------------------------------------------

#include <Win/WinMaker.h>
#include <Win/WinEx.h>
#include <Win/Win.h>
#include <assert.h>

namespace Win
{
	class SimpleControl : public Win::Dow::Handle
	{
	public:
		SimpleControl (Win::Dow::Handle winParent, int id)
			: Win::Dow::Handle (::GetDlgItem (winParent.ToNative (), id))
		{
			assert (winParent == GetParent ());
			assert (id == GetId ());
		}
		SimpleControl (Win::Dow::Handle win = 0)
			: Win::Dow::Handle (win)
		{}
		void Reset (Win::Dow::Handle h) { Win::Dow::Handle::Reset (h.ToNative ()); }
		void Init (Win::Dow::Handle winParent, int id)
		{
			Reset (::GetDlgItem (winParent.ToNative (), id));
			assert (!IsNull ());
			assert (winParent == GetParent ());
			assert (id == GetId ());
		}
		// code is the HIWORD (wParam)
		static bool IsClicked (int code)
		{
			return code == BN_CLICKED;
		}
	};

	class ControlMaker: public Win::ChildMaker
	{
	public:
		//	No edge--Use AddClientEdge() if needed
		ControlMaker (char const * className, Win::Dow::Handle winParent, int id)
			: Win::ChildMaker (className, winParent, id)
		{
			_style << Win::Style::Visible;
		}
		// Notice: controls are children of other windows and are automatically
		// destroyed when their parents are destroyed
		using Win::ChildMaker::Create;
	};
}

#endif
