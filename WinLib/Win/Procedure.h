#if !defined (PROCEDURE_H)
#define PROCEDURE_H
//----------------------------------------------------
// Procedure.h
// (c) Reliable Software 2000
//
//----------------------------------------------------

namespace Win
{
	// Window procedures

	typedef LRESULT (CALLBACK * ProcPtr)
		(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);


	LRESULT CALLBACK Procedure
		(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	LRESULT CALLBACK SubProcedure
		(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
}

#endif
