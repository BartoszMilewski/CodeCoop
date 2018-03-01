//----------------------------------
// (c) Reliable Software 2001 - 2008
//----------------------------------

#include <WinLibBase.h>
#include "DragDrop.h"

#include <Sys/Clipboard.h>
#include <Win/Handles.h>
#include <Com/Shell.h>

std::ostream& operator<<(std::ostream& os, FORMATETC const * fmt);
std::ostream& operator<<(std::ostream& os, STGMEDIUM const * stgMedium);
std::ostream& operator<<(std::ostream& os, REFIID ifaceId);

namespace Win
{
	//
	// Helper classes used by IDataObject interface
	//

	enum MediumType
	{
		Memory = TYMED_HGLOBAL, 
		DiskFile = TYMED_FILE, 
		Stream = TYMED_ISTREAM, 
		Storage = TYMED_ISTORAGE, 
		GDIBitmap = TYMED_GDI, 
		MetaFile = TYMED_MFPICT, 
		EnhMetaFile = TYMED_ENHMF, 
		Null = TYMED_NULL 
	};

	class FormatEtc : public FORMATETC
	{
	public:
		enum Aspect
		{
			Content = DVASPECT_CONTENT, 
			Thumbnail = DVASPECT_THUMBNAIL, 
			Icon = DVASPECT_ICON, 
			Print = DVASPECT_DOCPRINT 
		};

		FormatEtc (Clipboard::Format clipFmt, MediumType medium = Memory)
		{
			cfFormat = clipFmt;
			ptd = 0;
			dwAspect = Content;
			lindex = -1;
			tymed = medium;
		}

		FormatEtc (unsigned int registeredFmt, MediumType medium = Memory)
		{
			cfFormat = registeredFmt;
			ptd = 0;
			dwAspect = Content;
			lindex = -1;
			tymed = medium;
		}

		bool IsEqualClipboardFormat (unsigned int clipFmt) const
		{
			return cfFormat == clipFmt;
		}
		bool IsEqualMediaType (unsigned int mediaType) const
		{
			return (tymed & mediaType) != 0;
		}
		bool IsEqual (FORMATETC const * fmt) const
		{
			return IsEqualClipboardFormat (fmt->cfFormat) && IsEqualMediaType (fmt->tymed);
		}

		bool IsMemory () const { return (tymed & Memory) != 0; }
		bool IsFile () const { return (tymed & DiskFile) != 0; }
		bool IsStream () const { return (tymed & Stream) != 0; }
		bool IsStorage () const { return (tymed & Storage) != 0; }
		bool IsBitmap () const { return (tymed & GDIBitmap) != 0; }
		bool IsMetaFile () const { return (tymed & MetaFile) != 0; }
		bool IsEnhMetaFile () const { return (tymed & EnhMetaFile) != 0; }
		bool IsNull () const { return (tymed & Null) != 0; }
	};

	class StorageMedium : public STGMEDIUM
	{
	public:
		StorageMedium ()
		{
			tymed = TYMED_NULL;
			hGlobal = 0;
			pUnkForRelease = 0;
		}

		StorageMedium (MediumType medium, Win::Handle<HGLOBAL> handle, IUnknown * unkForRelease = 0)
		{
			tymed = medium;
			hGlobal = handle.ToNative ();
			pUnkForRelease = unkForRelease;
		}

		bool IsMemory () const { return (tymed & Memory) != 0; }
		bool IsFile () const { return (tymed & DiskFile) != 0; }
		bool IsStream () const { return (tymed & Stream) != 0; }
		bool IsStorage () const { return (tymed & Storage) != 0; }
		bool IsBitmap () const { return (tymed & GDIBitmap) != 0; }
		bool IsMetaFile () const { return (tymed & MetaFile) != 0; }
		bool IsEnhMetaFile () const { return (tymed & EnhMetaFile) != 0; }
		bool IsNull () const { return (tymed & Null) != 0; }

		HGLOBAL GetHandle () const
		{
			Assert (IsMemory ());
			return hGlobal;
		}
	};

	//
	// OLE file drop
	//

	class OleFileDrop : public IDataObject
	{
	public:
		OleFileDrop (std::vector<std::string> const & files);

		// IUnknown interface
		STDMETHODIMP QueryInterface (REFIID ifaceId, void ** ifacePtr);
		STDMETHODIMP_(ULONG) AddRef () { return ++_refCount; }
		STDMETHODIMP_(ULONG) Release ();

		// IDataObject interface
		STDMETHODIMP DAdvise (FORMATETC * fmt, DWORD flags, IAdviseSink * sink, DWORD * connection);
		STDMETHODIMP DUnadvise (DWORD connection);
		STDMETHODIMP EnumDAdvise (IEnumSTATDATA ** enumAdvise);
		STDMETHODIMP EnumFormatEtc (DWORD direction, IEnumFORMATETC ** enumFormatEtc);
		STDMETHODIMP GetCanonicalFormatEtc (FORMATETC * in, FORMATETC * out);
		STDMETHODIMP GetData (FORMATETC * fmt, STGMEDIUM * medium);
		STDMETHODIMP GetDataHere (FORMATETC * fmt, STGMEDIUM * medium);
		STDMETHODIMP QueryGetData (FORMATETC * fmt);
		STDMETHODIMP SetData (FORMATETC * fmt, STGMEDIUM * medium, BOOL release);

	private:
		class FormatEnumerator : public IEnumFORMATETC
		{
		public:
			FormatEnumerator (std::vector<FormatEtc> const & supportedFormats, unsigned int start = 0)
				: _refCount (1),
				  _supportedFormats (supportedFormats),
				  _cur (start)
			{}

			// IUnknown interface
			STDMETHODIMP QueryInterface (REFIID ifaceId, void ** ifacePtr);
			STDMETHODIMP_(ULONG) AddRef () { return ++_refCount; }
			STDMETHODIMP_(ULONG) Release ();

			// IEnumFORMATETC interface
			STDMETHODIMP Clone (IEnumFORMATETC ** clone);
			STDMETHODIMP Next (ULONG celt, FORMATETC *rgelt, ULONG *pceltFetched);
			STDMETHODIMP Reset ();
			STDMETHODIMP Skip (ULONG celt);

		private:
			unsigned long					_refCount;
			std::vector<FormatEtc> const &	_supportedFormats;
			unsigned int					_cur;
		};

	private:
		HRESULT IsSupported (FORMATETC const * fmt, std::vector<FormatEtc> const & formats) const;

	private:
		unsigned long			_refCount;
		std::vector<GlobalMem>	_packages;
		std::vector<FormatEtc>	_getFormats;
		std::vector<FormatEtc>	_setFormats;
	};

	OleFileDrop::OleFileDrop (std::vector<std::string> const & files)
		: _refCount (1)
	{
		// Create supported GET formats
		FormatEtc clipboard (Clipboard::FileDrop, Memory);
		_getFormats.push_back (clipboard);

		// Create packages for supported GET formats
		_packages.resize (_getFormats.size ());
		Clipboard::MakeFileDropPackage (files, _packages [0]);

		// Create SET formats -- currently we don't support any set formats
	}

	//
	// OleFileDrop::IUnknown interface
	//

	STDMETHODIMP OleFileDrop::QueryInterface (REFIID ifaceId, void ** ifacePtr)
	{
		dbg << "OleFileDrop::QueryInterface (" << ifaceId << ")" << std::endl;
		if (IsEqualIID (ifaceId, IID_IUnknown) || IsEqualIID (ifaceId, IID_IDataObject))
		{
			*ifacePtr = this;
			AddRef ();
			dbg << "    Reference count = " << _refCount << std::endl;
			return S_OK;
		}

		*ifacePtr = 0;
		dbg << "    Reference count = " << _refCount << std::endl;
		return E_NOINTERFACE;
	}

	STDMETHODIMP_(ULONG) OleFileDrop::Release ()
	{
		dbg << "OleFileDrop::Release -- _refCount == " << _refCount << std::endl;
		Assert (_refCount != 0);
		--_refCount;
		unsigned long refCount = _refCount;
		if (_refCount == 0)
			delete this;
		return refCount;
	}

	//
	// OleFileDrop::IDataObject interface
	//

	STDMETHODIMP OleFileDrop::DAdvise (FORMATETC * fmt, DWORD flags, IAdviseSink * sink, DWORD * connection)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP OleFileDrop::DUnadvise (DWORD connection)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP OleFileDrop::EnumDAdvise (IEnumSTATDATA ** enumAdvise)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP OleFileDrop::EnumFormatEtc (DWORD direction, IEnumFORMATETC ** enumFormatEtc)
	{
		dbg << "OleFileDrop::EnumFormatEtc -- direction: ";
		if (enumFormatEtc == 0)
			return E_INVALIDARG;

		if (direction == DATADIR_SET)
		{
			dbg << "SET" << std::endl;
			*enumFormatEtc = new FormatEnumerator (_setFormats);
			return S_OK;
		}
		else if (direction == DATADIR_GET)
		{
			dbg << "GET" << std::endl;
			*enumFormatEtc = new FormatEnumerator (_getFormats);
			return S_OK;
		}

		return E_INVALIDARG;
	}

	STDMETHODIMP OleFileDrop::GetCanonicalFormatEtc (FORMATETC * in, FORMATETC * out)
	{
		if (in == 0 || out == 0)
			return E_INVALIDARG;

		dbg << "OleFileDrop::GetCanonicalFormatEtc -- in " << in << std::endl;

		// For data objects that never provide device-specific renderings,
		// the simplest implementation of this method is to copy the input FORMATETC
		// to the output FORMATETC, store a NULL in the ptd field of the output FORMATETC,
		// and return DATA_S_SAMEFORMATETC
		out->cfFormat = in->cfFormat;
		out->dwAspect = in->dwAspect;
		out->lindex = in->lindex;
		out->ptd = 0;
		out->tymed = in->tymed;
		return DATA_S_SAMEFORMATETC;
	}

	STDMETHODIMP OleFileDrop::GetData (FORMATETC * fmt, STGMEDIUM * medium)
	{
		if (fmt == 0 || medium == 0)
			return E_INVALIDARG;

		dbg << "OleFileDrop::GetData -- " << fmt << std::endl;

		if (IsSupported (fmt, _getFormats) == S_OK)
		{
			medium->tymed = Memory;
			// The caller is resposible for releasing data
			medium->pUnkForRelease = 0;
			for (unsigned int i = 0; i < _getFormats.size (); i++)
			{
				if (_getFormats [i].IsEqual (fmt))
				{
					medium->hGlobal = ::OleDuplicateData (_packages [i].GetHandle (), fmt->cfFormat, 0);
					return S_OK;
				}
			}
			Assert (!"OleFileDrop::GetData -- should never happen");
		}

		return DV_E_FORMATETC;
	}

	STDMETHODIMP OleFileDrop::GetDataHere (FORMATETC * fmt, STGMEDIUM * medium)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP OleFileDrop::QueryGetData (FORMATETC * fmt)
	{
		if (fmt == 0)
			return E_INVALIDARG;

		dbg << "OleFileDrop::QueryGetData -- " << fmt << std::endl;

		return IsSupported (fmt, _getFormats);
	}

	STDMETHODIMP OleFileDrop::SetData (FORMATETC * fmt, STGMEDIUM * medium, BOOL release)
	{
		dbg << "OleFileDrop::SetData -- " << fmt << std::endl;

		if (IsSupported (fmt, _setFormats) == S_OK)
		{
			if (release == TRUE)
			{
				::ReleaseStgMedium (medium);
			}
			return S_OK;
		}
		return DV_E_FORMATETC;
	}

	class IsEqualFormat : public std::unary_function<FormatEtc const &, bool>
	{
	public:
		IsEqualFormat (FORMATETC const * fmt)
			: _fmt (fmt)
		{}

		bool operator () (FormatEtc const & format) const
		{
			return format.IsEqual (_fmt);
		}

	private:
		FORMATETC const * _fmt;
	};

	class IsEqualClipboardFormat : public std::unary_function<FormatEtc const &, bool>
	{
	public:
		IsEqualClipboardFormat (unsigned int clipFmt)
			: _clipFmt (clipFmt)
		{}

		bool operator () (FormatEtc const & format) const
		{
			return format.IsEqualClipboardFormat (_clipFmt);
		}

	private:
		unsigned int _clipFmt;
	};

	class IsEqualMediaType : public std::unary_function<FormatEtc const &, bool>
	{
	public:
		IsEqualMediaType (unsigned int mediaType)
			: _mediaType (mediaType)
		{}

		bool operator () (FormatEtc const & format) const
		{
			return format.IsEqualMediaType (_mediaType);
		}

	private:
		unsigned int _mediaType;
	};

	HRESULT OleFileDrop::IsSupported (FORMATETC const * fmt, std::vector<FormatEtc> const & formats) const
	{
		// Support others if needed DVASPECT_THUMBNAIL, DVASPECT_ICON, DVASPECT_DOCPRINT
		if (!(DVASPECT_CONTENT & fmt->dwAspect))
		{
			dbg << "    Aspect NOT supported" << std::endl;
			return DV_E_DVASPECT;
		}

		std::vector<FormatEtc>::const_iterator iter;
		iter = std::find_if (formats.begin (), formats.end (), IsEqualFormat (fmt));
		if (iter != formats.end ())
		{
			dbg << "    Format supported" << std::endl;
			return S_OK;
		}

		iter = std::find_if (formats.begin (), formats.end (), IsEqualClipboardFormat (fmt->cfFormat));
		if (iter != formats.end ())
		{
			dbg << "    Wrong media type" << std::endl;
			return DV_E_TYMED;
		}

		iter = std::find_if (formats.begin (), formats.end (), IsEqualMediaType (fmt->tymed));
		if (iter != formats.end ())
		{
			dbg << "    Wrong clipboard format" << std::endl;
			return DV_E_CLIPFORMAT;
		}

		dbg << "    Format NOT supported" << std::endl;
		return DV_E_FORMATETC;
	}

	//
	// OleFileDrop::FormatEnumerator IUnknow interface
	//

	STDMETHODIMP OleFileDrop::FormatEnumerator::QueryInterface (REFIID ifaceId, void ** ifacePtr)
	{
		dbg << "OleFileDrop::FormatEnumerator::QueryInterface (" << ifaceId << ")" << std::endl;
		if (IsEqualIID (ifaceId, IID_IUnknown) || IsEqualIID (ifaceId, IID_IEnumFORMATETC))
		{
			*ifacePtr = this;
			AddRef ();
			return S_OK;
		}

		*ifacePtr = 0;
		return E_NOINTERFACE;
	}

	STDMETHODIMP_(ULONG) OleFileDrop::FormatEnumerator::Release ()
	{
		dbg << "OleFileDrop::FormatEnumerator::Release -- _refCount == " << _refCount << std::endl;
		Assert (_refCount != 0);
		--_refCount;
		unsigned long refCount = _refCount;
		if (_refCount == 0)
			delete this;
		return refCount;
	}

	//
	// OleFileDrop::FormatEnumerator IEnumFORMATETC interface
	//
	 
	STDMETHODIMP OleFileDrop::FormatEnumerator::Clone (IEnumFORMATETC ** clone)
	{
		dbg << "OleFileDrop::FormatEnumerator::Clone" << std::endl;
		if (clone == 0)
			return E_INVALIDARG;

		*clone = new FormatEnumerator (_supportedFormats, _cur);
		return S_OK;
	}

	STDMETHODIMP OleFileDrop::FormatEnumerator::Next (ULONG celt, FORMATETC *rgelt, ULONG *pceltFetched)
	{
		dbg << "OleFileDrop::FormatEnumerator::Next -- request = " << celt << "; current = " << _cur << std::endl;
		if (celt == 0 || rgelt == 0)
			return E_INVALIDARG;

		if (pceltFetched == 0 && celt != 1) // pceltFetched can be NULL only for 1 item request
		  return E_INVALIDARG;

		if (_cur >= _supportedFormats.size ())
			return S_FALSE;		// No more formats

		unsigned long copied = 0;
		for ( ; _cur < _supportedFormats.size () && copied < celt; _cur++, copied++)
		{
			rgelt [copied] = _supportedFormats [_cur];
			dbg << "    Returning format - " << &rgelt [copied] << std::endl;
		}
		if (pceltFetched != 0)
			*pceltFetched = copied;
		return (celt == copied) ? S_OK : S_FALSE;
	}

	STDMETHODIMP OleFileDrop::FormatEnumerator::Reset ()
	{
		dbg << "OleFileDrop::FormatEnumerator::Reset" << std::endl;
		_cur = 0;
		return S_OK;
	}

	STDMETHODIMP OleFileDrop::FormatEnumerator::Skip (ULONG celt)
	{
		dbg << "OleFileDrop::FormatEnumerator::Skip" << std::endl;
		if ((_cur + celt) >= _supportedFormats.size ())
			return S_FALSE;
		_cur += celt;
		return S_OK;
	}

	//
	// DropSource
	//

	class DropSource : public IDropSource
	{
	public:
		DropSource (bool isRightButtonDrag)
			: _refCount (1),
			  _isRightButtonDrag (isRightButtonDrag)
		{}

		// IUnknown interface
		STDMETHODIMP QueryInterface (REFIID ifaceId, void ** ifacePtr);
		STDMETHODIMP_(ULONG) AddRef () { return ++_refCount; }
		STDMETHODIMP_(ULONG) Release ();

		// IDropSource interface
		STDMETHODIMP GiveFeedback (DWORD dwEffect);
		STDMETHODIMP QueryContinueDrag (BOOL fEscapePressed, DWORD grfKeyState);

	private:
		unsigned long	_refCount;
		bool			_isRightButtonDrag;
	};

	STDMETHODIMP DropSource::QueryInterface (REFIID ifaceId, void ** ifacePtr)
	{
		dbg << "DropSource::QueryInterface (" << ifaceId << ")" << std::endl;
		if (IsEqualIID (ifaceId, IID_IUnknown) || IsEqualIID (ifaceId, IID_IDropSource))
		{
			*ifacePtr = this;
			AddRef ();
			return S_OK;
		}

		*ifacePtr = 0;
		return E_NOINTERFACE;
	}

	STDMETHODIMP_(ULONG) DropSource::Release ()
	{
		dbg << "DropSource::Release -- _refCount == " << _refCount << std::endl;
		Assert (_refCount != 0);
		--_refCount;
		unsigned long refCount = _refCount;
		if (_refCount == 0)
			delete this;
		return refCount;
	}

	//
	// DropSource::IDropSource interface
	//

	STDMETHODIMP DropSource::GiveFeedback (DWORD dwEffect)
	{
		return DRAGDROP_S_USEDEFAULTCURSORS;
	}

	STDMETHODIMP DropSource::QueryContinueDrag (BOOL fEscapePressed, DWORD grfKeyState)
	{
		if (fEscapePressed == TRUE)
			return DRAGDROP_S_CANCEL;		// User pressed ESC -- cancel drag

		if (_isRightButtonDrag)
		{
			// Right mouse button drag
			if (!(grfKeyState & MK_RBUTTON))
				return DRAGDROP_S_DROP;		// User released right button -- perform drop
		}
		else
		{
			// Left mouse button drag
			if (grfKeyState & MK_RBUTTON)
				return DRAGDROP_S_CANCEL;	// User pressed right button -- cancel drag

			if (!(grfKeyState & MK_LBUTTON))
				return DRAGDROP_S_DROP;		// User released left button -- perform drop
		}

		return S_OK;						// Continue drag
	}
};

//
// Clipboard file drop
//

Win::FileDropHandle::FileDropHandle (Clipboard const & clipboard)
	: Handle<HDROP> (clipboard.GetFileDrop ())
{}

Win::FileDropHandle::FileDropHandle (Win::StorageMedium const & storageMedium)
	: Handle<HDROP> (reinterpret_cast<HDROP>(storageMedium.GetHandle ()))
{}

Win::FileDropHandle::FileDropHandle (unsigned long handle)
	: Handle<HDROP> (reinterpret_cast<HDROP>(handle))
{}

Win::FileDropHandle::FileDropHandle (HDROP handle)
	: Handle<HDROP> (handle)
{}

Win::FileDropHandle::Sequencer::Sequencer (FileDropHandle const & fileDrop)
	: _handle (fileDrop),
	  _cur (-1)
{
	_count = ::DragQueryFile (_handle.ToNative (), 0xffffffff, 0, 0);
	Advance ();
}

void Win::FileDropHandle::Sequencer::Advance ()
{
	Assert (!AtEnd ());
	_cur++;
	if (!AtEnd ())
	{
		unsigned int len = ::DragQueryFile (_handle.ToNative (), _cur, 0, 0) + 1;
		_path.resize (len);
		::DragQueryFile (_handle.ToNative (), _cur, &_path [0], len);
	}
}

//
// FileDropSink
//
Win::FileDropSink::FileDropSink()
#pragma warning (disable:4355)
	: _dropTarget (new Win::FileDropSink::Target (*this))
#pragma warning (default:4355)
{
}

void Win::FileDropSink::RegisterAsDropTarget(Win::Dow::Handle win)
{
	::RegisterDragDrop (win.ToNative (),	// Handle to a window that can accept drops
						_dropTarget);		// Pointer to object that is to be target of drop
}


//
// FileDropSink::Target
//

STDMETHODIMP Win::FileDropSink::Target::QueryInterface (REFIID ifaceId, void ** ifacePtr)
{
	dbg << "FileDropTarget::QueryInterface (" << ifaceId << ")" << std::endl;
	if (IsEqualIID (ifaceId, IID_IUnknown) || IsEqualIID (ifaceId, IID_IDropTarget))
	{
		*ifacePtr = this;
		AddRef ();
		return S_OK;
	}

	*ifacePtr = 0;
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) Win::FileDropSink::Target::Release ()
{
	// dbg << "FileDropTarget::Release -- _refCount == " << _refCount << std::endl;
	Assert (_refCount != 0);
	--_refCount;
	unsigned long refCount = _refCount;
	if (_refCount == 0)
		delete this;
	return refCount;
}

//
// FileDropTarget::IDropTarget interface
//

STDMETHODIMP Win::FileDropSink::Target::DragEnter (IDataObject * pDataObject,	// Pointer to the interface of the source data object
												   DWORD grfKeyState,			// Current state of keyboard modifier keys
												   POINTL pt,					// Current cursor coordinates (in screen coordinates)
												   DWORD * pdwEffect)			// Pointer to the effect of the drag-and-drop operation
{
	dbg << "FileDropTarget::DragEnter" << std::endl;
	if (pDataObject == 0 || pdwEffect == 0)
		return E_INVALIDARG;

	Win::FormatEtc acceptedFormat (Clipboard::FileDrop);
	HRESULT result = pDataObject->QueryGetData (&acceptedFormat);
	if (result == S_OK)
	{
		// Data object contains a data in the format we can accept
		*pdwEffect = DROPEFFECT_COPY;	// We always copy dropped files
		Win::Point dropPoint (pt.x, pt.y);
		_dropSink.OnDragEnter (dropPoint);
	}
	return result;
}

STDMETHODIMP Win::FileDropSink::Target::DragLeave (void)
{
	dbg << "FileDropTarget::DragLeave" << std::endl;
	_dropSink.OnDragLeave ();
	return S_OK;
}

STDMETHODIMP Win::FileDropSink::Target::DragOver (DWORD grfKeyState,	// Current state of keyboard modifier keys
												  POINTL pt,			// Current cursor coordinates (in screen coordinates)
												  DWORD * pdwEffect)	// Pointer to the effect of the drag-and-drop operation
{
	//dbg << "FileDropTarget::DragOver" << std::endl;
	if (pdwEffect == 0)
		return E_INVALIDARG;

	*pdwEffect = DROPEFFECT_COPY;	// We always copy dropped files
	Win::Point dropPoint (pt.x, pt.y);
	_dropSink.OnDragOver (dropPoint);
	return S_OK;
}

STDMETHODIMP Win::FileDropSink::Target::Drop (IDataObject * pDataObject,	// Pointer to the interface for the source data
											  DWORD grfKeyState,			// Current state of keyboard modifier keys
											  POINTL pt,					// Current cursor coordinates (in screen coordinates)
											  DWORD * pdwEffect)			// Pointer to the effect of the drag-and-drop operation
{
	dbg << "FileDropTarget::Drop" << std::endl;
	if (pDataObject == 0 || pdwEffect == 0)
		return E_INVALIDARG;

	Win::FormatEtc acceptedFormat (Clipboard::FileDrop);
	Win::StorageMedium storageMedium;
	HRESULT result = pDataObject->GetData (&acceptedFormat, &storageMedium);
	if (result == S_OK)
	{
		// Got data in the requested format
		Win::FileDropHandle droppedFiles (storageMedium);
		Win::Point dropPoint (pt.x, pt.y);
		// We ignore keyboard keys and drop effect, because we will figure
		// if dropped files have to be copied or moved. We don't support
		// dropping file links.
		_dropSink.OnFileDrop (droppedFiles, dropPoint);
	}
	return result;
}

//
// OLE File Drag&Drop
//

Win::FileDragger::FileDragger (std::vector<std::string> const & files,
								 bool isRightButtonDrag)
	: _fileDrop (new OleFileDrop (files)),
	  _dropSource (new DropSource (isRightButtonDrag)),
	  _dropResult (0)
{}

void Win::FileDragger::Do ()
{
	HRESULT result = ::DoDragDrop (_fileDrop, _dropSource, DROPEFFECT_COPY, &_dropResult);
}

#if !defined (NDEBUG)
std::ostream& operator<<(std::ostream& os, FORMATETC const * fmt)
{
	Win::FormatEtc const * format = reinterpret_cast<Win::FormatEtc const *>(fmt);
	os << "Format = (";
	switch (fmt->cfFormat)
	{
	case Clipboard::Bitmap:
		os << "Bitmap";
		break;
	case Clipboard::Dib:
		os << "Dib";
		break;
	case Clipboard::Dif:
		os << "Dif";
		break;
	case Clipboard::EnhancedMetaFile:
		os << "EnhancedMetaFile";
		break;
	case Clipboard::MetaFilePicture:
		os << "MetaFilePicture";
		break;
	case Clipboard::OemText:
		os << "OemText";
		break;
	case Clipboard::Palette:
		os << "Palette";
		break;
	case Clipboard::PenData:
		os << "PenData";
		break;
	case Clipboard::Riff:
		os << "Riff";
		break;
	case Clipboard::Sylk:
		os << "Sylk";
		break;
	case Clipboard::Text:
		os << "Text";
		break;
	case Clipboard::Tiff:
		os << "Tiff";
		break;
	case Clipboard::Wave:
		os << "Wave";
		break;
	case Clipboard::UnicodeText:
		os << "UnicodeText";
		break;
	case Clipboard::FileDrop:
		os << "FileDrop";
		break;
	default:
		{
			if (fmt->cfFormat > 0xc000)
			{
				char buf [512];
				memset (buf, 0, sizeof (buf));
				if (::GetClipboardFormatName (fmt->cfFormat, buf, sizeof (buf)) != 0)
					dbg << buf;
				else
					dbg << "Cannot registered format name";
				dbg << " (0x" << std::hex << fmt->cfFormat << ")";
			}
			else
				os << "0x" << std::hex << fmt->cfFormat;
		}
		break;
	}

	os << ", ";
	if (format->IsMemory ())
		os << "memory";
	else if (format->IsFile ())
		os << "disk file";
	else if (format->IsStream ())
		os << "stream";
	else if (format->IsStorage ())
		os << "storage";
	else if (format->IsBitmap ())
		os << "bitmap";
	else if (format->IsMetaFile ())
		os << "metafile";
	else if (format->IsEnhMetaFile ())
		os << "enhanced metafile";
	else if (format->IsNull ())
		os << "null";
	else
		os << "UNKNOWN";
	os << ")";

	return os;
}

std::ostream& operator<<(std::ostream& os, STGMEDIUM const * stgMedium)
{
	Win::StorageMedium const * medium = reinterpret_cast<Win::StorageMedium const *>(stgMedium);
	os << "Storage Medium = ";
	if (medium->IsMemory ())
		os << "memory";
	else if (medium->IsFile ())
		os << "disk file";
	else if (medium->IsStream ())
		os << "stream";
	else if (medium->IsStorage ())
		os << "storage";
	else if (medium->IsBitmap ())
		os << "bitmap";
	else if (medium->IsMetaFile ())
		os << "metafile";
	else if (medium->IsEnhMetaFile ())
		os << "enhanced metafile";
	else if (medium->IsNull ())
		os << "null";
	else
		os << "UNKNOWN";

	return os;
}

std::ostream& operator<<(std::ostream& os, REFIID ifaceId)
{
	if (IsEqualIID (ifaceId, IID_IUnknown))
	{
		os << "IID_IUnknown";
	}
	else if (IsEqualIID (ifaceId, IID_IDataObject))
	{
		os << "IID_IDataObject";
	}
	else if (IsEqualIID (ifaceId, IID_IEnumFORMATETC))
	{
		os << "IDD_IEnumFORMATETC";
	}
	else if (IsEqualIID (ifaceId, IID_IDropSource))
	{
		os << "IDD_IDropSource";
	}
	else
	{
		os << "NOT Implemented";
	}
	return os;
}

#endif
