#if !defined (CLIPBOARD_H)
#define CLIPBOARD_H
//
// (c) Reliable Software, 1998 -- 2003
//

class GlobalMem;

namespace Win
{
	class FileDropHandle;
}

class Clipboard
{
	friend class Win::FileDropHandle;

public:
	Clipboard (Win::Dow::Handle win)
		: _winOwner (win)
	{}

	enum Format
	{
		Bitmap = CF_BITMAP,
		Dib = CF_DIB,
		Dif = CF_DIF,
		EnhancedMetaFile = CF_ENHMETAFILE,
		MetaFilePicture = CF_METAFILEPICT,
		OemText = CF_OEMTEXT,
		Palette = CF_PALETTE,
		PenData = CF_PENDATA,
		Riff = CF_RIFF,
		Sylk = CF_SYLK,
		Text = CF_TEXT,
		Tiff = CF_TIFF,
		Wave = CF_WAVE,
		UnicodeText = CF_UNICODETEXT,
		FileDrop = CF_HDROP
	};

	static bool IsFormatText () { return ::IsClipboardFormatAvailable (CF_TEXT) != 0; }
	static bool IsFormatFileDrop () { return ::IsClipboardFormatAvailable (CF_HDROP) != 0; }

	void Clear ();

	bool HasText () const;
	void PutText (char const * text, int len);
	HGLOBAL GetText () const;
	void PutFileDrop (std::vector<std::string> const & files);
	static void MakeFileDropPackage (std::vector<std::string> const & files, GlobalMem & package);

private:
	HDROP GetFileDrop () const;

private:
	class Lock
	{
	public:
		Lock (Win::Dow::Handle winOwner)
		{
			if (::OpenClipboard (winOwner.ToNative ()) == 0)
				throw Win::Exception ("Cannot open clipboard, because it is used by some other application.");
		}
		~Lock ()
		{
			::CloseClipboard ();
		}
	};

private:
	Win::Dow::Handle _winOwner;
};

#endif
