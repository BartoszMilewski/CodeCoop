#if !defined (WINMAKER_H)
#define WINMAKER_H
//-----------------------------------------
// Copyright Reliable Software 2000 -- 2002
//-----------------------------------------
#include <Sys/WinString.h>

namespace DDE
{
	class Controller;
}

namespace Win
{
	class Controller;
	class TopController;

	class Maker
	{
	public:
		Maker (int idClassName, Win::Instance hInstance);
		Maker (char const * className, Win::Instance hInstance);
		Win::Style & Style () { return _style; }
		void SetPosition (int x, int y, int width, int height);
		void SetSize (int width, int height)
		{
			_width = width;
			_height = height;
		}
		void AddCaption (char const * caption)
		{
			_windowName = caption;
		}
		void AddCreationData (void * data) { _data = data; }
		void SetOwner (Win::Dow::Handle hwndOwner) {_hWndParent = hwndOwner; }

		// Notice: Create returns handle, not auto handle (Win::Dow::Owner) 
		// since most windows are destroyed in response to user actions (close, exit, etc...)
		Win::Dow::Handle Create (Win::Controller * control, char const * title = "");
		Win::Dow::Handle Create (std::unique_ptr<Win::Controller> control, char const * title = "");
		Win::Dow::Handle Create ();

		void Show (int nCmdShow = SW_SHOWNORMAL);

	protected:

		ResString   _classString;

		Win::Style		_style;		// style and extended style
		char const *_className;     // pointer to registered class name
		char const *_windowName;    // pointer to window name
		int         _x;             // horizontal position of window
		int         _y;             // vertical position of window
		int         _width;         // window width  
		int         _height;        // window height
		Win::Dow::Handle    _hWndParent;    // handle to parent or owner window
		HMENU       _hMenu;         // handle to menu, or child-window identifier
		HINSTANCE   _hInstance;     // handle to application instance
		void *      _data;          // pointer to window-creation data
	};

	class TopMaker: public Maker
	{
	public:
		TopMaker (char const * caption, int idClassName, Win::Instance hInst);
		TopMaker (char const * caption, char const * className, Win::Instance hInst);
	};

	class ChildMaker: public Maker
	{
	public:
		// Notice: child window is usually destroyed when the parent is destroyed
		ChildMaker (int idClassName, Win::Dow::Handle hwndParent, int childId);
		ChildMaker (char const * className, Win::Dow::Handle hwndParent, int childId);
		// Call only if you don't want to use winParent's instance
		void SetInstance (Win::Instance inst)
		{
			_hInstance = inst;
		}
	};

	class PopupMaker: public Maker
	{
	public:
		PopupMaker (int idClassName, Win::Instance hInst);
		PopupMaker (char const * className, Win::Instance hInst);
	};
};

#endif
