#if	!defined (KEYBOARD_H)
#define	KEYBOARD_H
//----------------------------------------------------
// Keyboard.h
// (c) Reliable	Software 2000-2002
//----------------------------------------------------

#include "Procedure.h"
#include "Dialog.h"

namespace Win 
{
	class Controller;
}

namespace Keyboard
{
	class Handler
	{
		friend class Win::Controller;
	public:
		virtual	~Handler ()	{}

		static bool	IsShift	() { return	(::GetKeyState (VK_SHIFT) &	0x8000)	!= 0; }
		static bool	IsCtrl () {	return (::GetKeyState (VK_CONTROL) & 0x8000) !=	0; }
		static bool	IsAlt () { return (::GetKeyState (VK_MENU) & 0x8000) !=	0; }

		virtual bool OnKeyDown (int	vKey, int flags);

		virtual	bool OnPageUp () throw ()
			{ return false;	}
		virtual	bool OnPageDown	() throw ()
			{ return false;	}
		virtual	bool OnUp () throw () 
			{ return false;	}
		virtual	bool OnDown	() throw ()	
			{ return false;	}
		virtual	bool OnLeft	() throw ()	
			{ return false;	}
		virtual	bool OnRight ()	throw ()
			{ return false;	}
		virtual	bool OnHome	() throw ()
			{ return false;	}
		virtual	bool OnEnd () throw	() 
			{ return false;	}
		virtual	bool OnReturn () throw ()
			{ return false;	}
		virtual	bool OnDelete () throw () 
			{ return false;	}
		virtual	bool OnBackSpace ()	throw ()
			{ return false;	}
		virtual	bool OnEscape () throw () 
			{ return false;	}
		virtual bool OnTab () throw ()
			{ return false; }
		virtual	bool OnCharKey (int	vKey) throw	() 
			{ return false;	}
		virtual	bool OnNumpad (int vKey) throw () 
			{ return false;	}
		virtual	bool OnFunctionKey (int	keyNo) throw ()	
			{ return false;	}
	};
}

namespace VKey
{
	enum
	{
		// Symbolic	constant	Value			Mouse or keyboard equivalent 
		LeftButton =			VK_LBUTTON,		// Left	mouse button  
		RightButton	=			VK_RBUTTON,		// Right mouse button  
		Cancel =				VK_CANCEL,		// Control-break processing	 
		MiddleButton =			VK_MBUTTON,		// Middle mouse	button (three-button mouse)	 
		BackSpace =				VK_BACK,		// BACKSPACE key  
		Tab	=					VK_TAB,			// TAB key	
		Clear =					VK_CLEAR,		// CLEAR key  
		Return =				VK_RETURN,		// ENTER key  
		Shift =					VK_SHIFT,		// SHIFT key  
		Control	=				VK_CONTROL,		// CTRL	key	 
		Alt	=					VK_MENU,		// ALT key	
		Pause =					VK_PAUSE,		// PAUSE key  
		CapsLock =				VK_CAPITAL,		// CAPS	LOCK key  
		Escape =				VK_ESCAPE,		// ESC key	
		Space =					VK_SPACE,		// SPACEBAR	 
		Prior =					VK_PRIOR,		// PAGE	UP key	
		Next =					VK_NEXT,		// PAGE	DOWN key  
		End	=					VK_END,			// END key	
		Home =					VK_HOME,		// HOME	key	 
		Left =					VK_LEFT,		// LEFT	ARROW key  
		Up =					VK_UP,			// UP ARROW	key	 
		Right =					VK_RIGHT,		// RIGHT ARROW key	
		Down =					VK_DOWN,		// DOWN	ARROW key  
		Select =				VK_SELECT,		// SELECT key
		Print =					VK_PRINT,		// PRINT key
		Execute	=				VK_EXECUTE,		// EXECUTE key	
		SnapShot =				VK_SNAPSHOT,	// PRINT SCREEN	key	for	Windows	3.0	and	later  
		Insert =				VK_INSERT,		// INS key	
		Delete =				VK_DELETE,		// DEL key	
		Help =					VK_HELP,		// HELP	key

		_0 =					'0',			// 0 key  
		_1 =					'1',			// 1 key
		_2 =					'2',			// 2 key  
		_3 =					'3',			// 3 key  
		_4 =					'4',			// 4 key  
		_5 =					'5',			// 5 key  
		_6 =					'6',			// 6 key  
		_7 =					'7',			// 7 key
		_8 =					'8',			// 8 key  
		_9 =					'9',			// 9 key  

		A =						'A',			// A key  
		B =						'B',			// B key
		C =						'C',			// C key
		D =						'D',			// D key
		E =						'E',			// E key
		F =						'F',			// F key
		G =						'G',			// G key
		H =						'H',			// H key
		I =						'I',			// I key
		J =						'J',			// J key
		K =						'K',			// K key
		L =						'L',			// L key
		M =						'M',			// M key
		N =						'N',			// N key
		O =						'O',			// O key
		P =						'P',			// P key
		Q =						'Q',			// Q key
		R =						'R',			// R key
		S =						'S',			// S key
		T =						'T',			// T key
		U =						'U',			// U key
		V =						'V',			// V key
		W =						'W',			// W key
		X =						'X',			// X key
		Y =						'Y',			// Y key
		Z =						'Z',			// Z key

		LeftWin	=				VK_LWIN,		// Left	Windows	key	(Microsoft Natural Keyboard)  
		RightWin =				VK_RWIN,		// Right Windows key (Microsoft	Natural	Keyboard)  
		Apps =					VK_APPS,		// Applications	key	(Microsoft Natural Keyboard)  

		Num0 =					VK_NUMPAD0,		// Numeric keypad 0	key	 
		Num1 =					VK_NUMPAD1,		// Numeric keypad 1	key
		Num2 =					VK_NUMPAD2,		// Numeric keypad 2	key
		Num3 =					VK_NUMPAD3,		// Numeric keypad 3	key
		Num4 =					VK_NUMPAD4,		// Numeric keypad 4	key
		Num5 =					VK_NUMPAD5,		// Numeric keypad 5	key
		Num6 =					VK_NUMPAD6,		// Numeric keypad 6	key
		Num7 =					VK_NUMPAD7,		// Numeric keypad 7	key
		Num8 =					VK_NUMPAD8,		// Numeric keypad 8	key
		Num9 =					VK_NUMPAD9,		// Numeric keypad 9	key
		Multiply =				VK_MULTIPLY,	// Multiply	key
		Add	=					VK_ADD,			// Add key	
		Separator =				VK_SEPARATOR,	// Separator key  
		Substract =				VK_SUBTRACT,	// Subtract	key
		Decimal	=				VK_DECIMAL,		// Decimal key
		Divide =				VK_DIVIDE,		// Divide key

		F1 =					VK_F1,			// F1 key
		F2 =					VK_F2,			// F2 key
		F3 =					VK_F3,			// F3 key
		F4 =					VK_F4,			// F4 key
		F5 =					VK_F5,			// F5 key
		F6 =					VK_F6,			// F6 key
		F7 =					VK_F7,			// F7 key
		F8 =					VK_F8,			// F8 key
		F9 =					VK_F9,			// F9 key
		F10	=					VK_F10,			// F10 key
		F11	=					VK_F11,			// F11 key
		F12	=					VK_F12,			// F12 key
		F13	=					VK_F13,			// F13 key
		F14	=					VK_F14,			// F14 key
		F15	=					VK_F15,			// F15 key
		F16	=					VK_F16,			// F16 key
		F17	=					VK_F17,			// F17 key
		F18	=					VK_F18,			// F18 key
		F19	=					VK_F19,			// F19 key
		F20	=					VK_F20,			// F20 key
		F21	=					VK_F21,			// F21 key
		F22	=					VK_F22,			// F22 key
		F23	=					VK_F23,			// F23 key
		F24	=					VK_F24,			// F24 key

		NumLock	=				VK_NUMLOCK,		// NUM LOCK	key	 
		ScrollLock =			VK_SCROLL,		// SCROLL LOCK key

		LeftShift =				VK_LSHIFT,		// Left	SHIFT
		RightShift =			VK_RSHIFT,		// Right SHIFT
		LeftCtrl =				VK_LCONTROL,	// Left	CONTROL
		RightCtrl =				VK_RCONTROL,	// Right CONTROL
		LeftAlt	=				VK_LMENU,		// Left	ALT
		RightAlt =				VK_RMENU,		// Right ALT

		Attn =					VK_ATTN,		// ATTN	key	 
		CrSel =					VK_CRSEL,		// CRSEL key
		ExSel =					VK_EXSEL,		// EXSEL key
		EraseEOF =				VK_EREOF,		// Erase EOF key
		Play =					VK_PLAY,		// PLAY	key
		Zoom =					VK_ZOOM,		// ZOOM	key
		NoName =				VK_NONAME,		// Reserved	for	future use	
		PA1	=					VK_PA1,			// PA1 key
		OEMClear =				VK_OEM_CLEAR	// CLEAR key
	};
}

#endif
