#if !defined CANVAS_H
#define CANVAS_H
//------------------------------------
//  (c) Reliable Software, 1996 - 2006
//------------------------------------

#include <Graph/Color.h>
#include <Win/Geom.h>
#include <Win/Region.h>

namespace Win
{
	class Rect;
	class Point;

	// A handle to Device Context
	class Canvas: public Win::Handle<HDC>
	{
	public:
		Canvas(HDC hdc) : Win::Handle<HDC> (hdc) {}
		Gdi::Handle SelectObject (Gdi::Handle newObject);

		void SetViewportOrg (int x, int y)
		{
			::SetViewportOrgEx (H (), x, y, 0);
		}

        void SetPixel (int x, int y, Win::Color color)
		{
            ::SetPixel (H (), x, y, color.ToNative ());
		}

        void MoveTo (int x, int y)
		{
            ::MoveToEx (H (), x, y, 0);
		}

	    // Line
        void LineTo (int x, int y)
		{
            ::LineTo (H (), x, y);
		}

        void Line (int x1, int y1, int x2, int y2)
		{
            ::MoveToEx (H (), x1, y1, 0);
            ::LineTo (H (), x2, y2);
		}

	    // Rectangle
        void Frame (Win::Rect const & rect, HPEN pen);
	    void Frame (Win::Rect const & rect, HBRUSH brush);

        void FillRect (Win::Rect const & rect, HBRUSH brush);

	    // Ellipse uses current pen and brush
	    void Ellipse (int left, int top, int right, int bottom)
		{
		    ::Ellipse (H (), left, top, right, bottom);
		}

		void DrawFocusRect(Win::Rect const rect)
		{
			::DrawFocusRect(H (), &rect);
		}

		// Text
		void Text (int x, int y, char const * buf, int size)
		{
			::TextOut (H (), x, y, buf, size);
		}

        int TabbedText (int x, int y, char const * buf, int cBuf, int tabSize, int xOrg = 0)
		{
            DWORD dim = ::TabbedTextOut (H (), x, y, buf, cBuf, 1, &tabSize, xOrg);
            return LOWORD (dim); // lenght in pixels
		}

        void GetCharSize (int& cxChar, int& cyChar);
        void GetTextSize (char const * text, int& cxText, int& cyText, size_t lenString = 0);

	    // Bitmap
	    void PaintBitmap (  Bitmap::Handle bitmap, 
						    int width, 
						    int height, 
						    int xDest = 0, 
						    int yDest = 0, 
						    int xSrc = 0, 
						    int ySrc = 0,
						    DWORD rop = SRCCOPY);

		void StretchBitmap (Bitmap::Handle bitmap,
						    int srcWidth,
						    int srcHeight,
						    int dstWidth,
						    int dstHeight,
						    int xDest = 0, 
						    int yDest = 0, 
						    int xSrc = 0, 
						    int ySrc = 0,
						    DWORD rop = SRCCOPY);
	};

    // Use for painting after WM_PAINT

    class PaintCanvas: public Canvas
	{
    public:
        PaintCanvas (Win::Dow::Handle win)
		    :   Canvas (::BeginPaint (win.ToNative (), &_paint)),
				_win (win)
		{}
        ~PaintCanvas ()
		{
			::EndPaint(_win.ToNative (), &_paint);
		}
		bool IsEraseBackground () const { return _paint.fErase != 0; }
        int Top () const    { return _paint.rcPaint.top; }
        int Bottom () const { return _paint.rcPaint.bottom; }
        int Left () const   { return _paint.rcPaint.left; }
        int Right () const  { return _paint.rcPaint.right; }

		void EraseBackground (Win::Color backgroundColor);

	private:
		PaintCanvas (PaintCanvas &);
		PaintCanvas & operator= (PaintCanvas &);

	protected:
        PAINTSTRUCT _paint;
        Win::Dow::Handle    _win;
	};

    // Device Context
    // Use for painting other than WM_PAINT

    class UpdateCanvas: public Canvas
	{
    public:
        UpdateCanvas (Win::Dow::Handle win)
		    :   Canvas (::GetDC(win.ToNative ())),
				_win(win)
		{
			Assert (!IsNull ());
		}
        ~UpdateCanvas ()
		{
            ::ReleaseDC (_win.ToNative (), H ());
		}
	private:
		UpdateCanvas (UpdateCanvas &);
		UpdateCanvas & operator= (UpdateCanvas &);
    protected:
        Win::Dow::Handle _win;
	};

	// Use when LockWindowUpdate is in effect
	class LockUpdateCanvas: public Canvas
	{
	public:
		LockUpdateCanvas (Win::Dow::Handle win, Region::Handle region)
		    :   Canvas (::GetDCEx(win.ToNative (), region.ToNative (), DCX_LOCKWINDOWUPDATE)),
				_win(win)
		{
			if (IsNull ())
				throw Win::Exception ("LockUpdateCanvas could not be created");
			Assert (!IsNull ());
		}
        ~LockUpdateCanvas ()
		{
            ::ReleaseDC (_win.ToNative (), H ());
		}
	private:
		LockUpdateCanvas (LockUpdateCanvas &);
		LockUpdateCanvas & operator= (LockUpdateCanvas &);
    protected:
        Win::Dow::Handle _win;
	};

    // For painting in memory
    class MemCanvas: public Canvas
	{
    public:
        MemCanvas (HDC hdc) 
            : Canvas (::CreateCompatibleDC (hdc))
		{}

        ~MemCanvas ()
		{
			::DeleteDC(H ()); 
		}
	private:
		MemCanvas (MemCanvas &);
		MemCanvas & operator= (MemCanvas &);
	};

	class DeviceCanvas: public Canvas
	{
	public:
		~DeviceCanvas ()
		{
			::DeleteDC (H ());
		}
	private:
		DeviceCanvas (DeviceCanvas &);
		DeviceCanvas & operator= (DeviceCanvas &);
	protected:
		DeviceCanvas (HDC hdc) : Canvas (hdc) {}
	};

	// For drawing on the desktop
	class DesktopCanvas : public DeviceCanvas
	{
	public:
		DesktopCanvas ()
			: DeviceCanvas (::CreateDC ("DISPLAY", 0, 0, 0))
		{}
	};

	class PrinterCanvas : public DeviceCanvas
	{
	public:
		PrinterCanvas ()
			: DeviceCanvas (::CreateDC ("WINSPOOL", 0, 0, 0))
		{}
	};
}

#endif
