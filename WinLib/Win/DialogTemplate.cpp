//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include <WinLibBase.h>
#include <Win/DialogTemplate.h>
#include <Win/Dialog.h>

void Dialog::Template::Load (Win::Instance inst, int dlgId)
{
	Resource<unsigned char> dlg1Res (inst, dlgId, RT_DIALOG);
	unsigned char * data = dlg1Res.Lock();
	std::copy (data, data + dlg1Res.GetSize (), std::back_inserter (_data));
	_version = ToEx ()->dlgVer;
	WORD signature = ToEx ()->signature;
	if (signature == 0xffff)
	{
		if (_version != 1)
		{
			throw Win::Exception ("Unknown version of dialog template");
		}
	}
	else
		_version = 0;
}

Dialog::TemplateMaker::TemplateMaker ()
	: _helpId (0),
	  _fontName (L"MS Sans Serif"),
	  _pointSize (8),
	  _weight (Font::Weight::DontCare),
	  _italic (false),
	  _charSet (Font::CharSet::Default)
{
	// Default dialog window styles
	_style << Dialog::Style::ModalFrame
		   << Dialog::Style::FixedSys
		   << Dialog::Style::SetFont
		   << Dialog::Style::Look3D
		   << Win::Style::Popup
		   << Win::Style::AddTitleBar;
	Font::GUIDefault guiFont;
	if (!guiFont.IsNull ())
	{
		Font::Descriptor guiFontDescriptor;
		::GetObject (guiFont.ToNative (), sizeof (Font::Descriptor), &guiFontDescriptor);
		_fontName = ::ToWString(guiFontDescriptor.GetFaceName ());
		_weight = guiFontDescriptor.GetWeight ();
		_italic = guiFontDescriptor.IsItalic ();
		_charSet = guiFontDescriptor.GetCharSet ();
		// Set size for the screen
		Win::DesktopCanvas canvas;
		int logpix = ::GetDeviceCaps (canvas.ToNative (), LOGPIXELSY);
		int height = guiFontDescriptor.GetHeight ();
		if (height < 0)
			height = -height;
		_pointSize = ::MulDiv (height, 72, logpix);
	}
	// else dialog font is 8 point 'MS Sans Serif'
}

void Dialog::TemplateMaker::SetFont (std::wstring const & name,
								     unsigned pointSize,
								     unsigned weight,
								     bool italic,
								     unsigned charSet)
{
	_style << Dialog::Style::SetFont;
	_fontName = name;
	_pointSize = pointSize;
	_weight = weight;
	_italic = italic;
	_charSet = charSet;
}

void Dialog::TemplateMaker::Item::Serialize (Dialog::Template::Serializer & out) const
{
	out.DwordAlign ();
	// DWORD helpID;
	out.PutDWord (_helpId);
	// DWORD exStyle;
	out.PutDWord (_style.GetExStyleBits ());
	// DWORD style;
	out.PutDWord (_style.GetStyleBits ());
	// short x, y, cx, cy;
	out.PutShort (_rect.Left ());
	out.PutShort (_rect.Top ());
	out.PutShort (_rect.Width ());
	out.PutShort (_rect.Height ());
	// DWORD id;
	out.PutDWord (_id);
	// sz_Or_Ord windowClass;
	if (_className.empty ())
	{
		out.PutWchar (0xffff);
		out.PutWchar (static_cast<wchar_t> (_classId));
	}
	else
	{
		out.PutString (_className);
	}
	// sz_Or_Ord title;
	out.PutString (_text);
	// WORD extraCount;
	out.PutWord (0);
}

void Dialog::TemplateMaker::Create (Dialog::Template & tmpl)
{
	tmpl.SetVersion (1); // Extended
	Dialog::Template::Serializer out = tmpl.GetSerializer ();
	// WORD dlgVer;
	out.PutWord (1);
	// WORD signature;
	out.PutWord (0xffff);
	// DWORD helpID;
	out.PutDWord (_helpId);
	// DWORD exStyle;
	out.PutDWord (_style.GetExStyleBits ());
	// DWORD style;
	out.PutDWord (_style.GetStyleBits ());
	// WORD cDlgItems;
	out.PutWord (_items.size ());
	// short x, y, cx, cy;
	out.PutShort (_rect.Left ());
	out.PutShort (_rect.Top ());
	out.PutShort (_rect.Width ());
	out.PutShort (_rect.Height ());
	// sz_Or_Ord menu;
	out.PutWchar (0);
	// sz_Or_Ord windowClass;
	out.PutWchar (0);
	// WCHAR title[titleLen];
	out.PutString (_title);
	if (_style.TestStyleBits (Dialog::Style::SetFont))
	{
		// These members are only present if the style specified DS_SETFONT or DS_SHELLFONT
		// WORD pointsize;
		out.PutWord (_pointSize);
		// WORD weight;
		out.PutWord (_weight);
		// BYTE italic;
		out.PutByte (_italic ? 1 : 0);
		// BYTE charset;
		out.PutByte (_charSet);
		// WCHAR typeface[stringLen];
		out.PutString (_fontName);
	}
	for (ItemList::const_iterator it = _items.begin (); it != _items.end (); ++it)
		(*it)->Serialize (out);
}
