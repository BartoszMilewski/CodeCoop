#if !defined (DRAGDROP_H)
#define DRAGDROP_H
//----------------------------------
// (c) Reliable Software 1998 - 2008
//----------------------------------

#include <Win/Geom.h>
#include <File/Path.h>
#include <Win/Handles.h>
#include <Sys/WinGlobalMem.h>
#include <Com/Com.h>

class Clipboard;

// Windows sends WM_DROPFILES only to windows that
// registered interest in drag and drop by calling
// Win::Dow::Handle::DragAcceptFiles () 




namespace Win
{
	class StorageMedium;

	// Access to file list stored by Windows in the Clipboard or during drag and drop
	class FileDropHandle: public Handle<HDROP>
	{
	public:
		class Sequencer : public PathSequencer
		{
		public:
			Sequencer (FileDropHandle const & fileDrop);

			unsigned int GetCount () const { return _count; }
			char const * GetFilePath () const { return _path.c_str (); }
			void Advance ();
			bool AtEnd () const { return _cur == _count; }

		private:
			Handle<HDROP>	_handle;
			unsigned int	_cur;
			unsigned int	_count;
			std::string		_path;
		};

		bool IsDropInClientArea () const
		{
			Win::Point pt;
			return GetDropPoint (pt);
		}
		// true if within client area
		bool GetDropPoint (Win::Point & pt) const
		{
			return ::DragQueryPoint (ToNative (), &pt) != FALSE;
		}
	public:
		FileDropHandle (Clipboard const & clipboard);
		FileDropHandle (Win::StorageMedium const & storageMedium);
		FileDropHandle (unsigned long handle = 0);

	protected:
		FileDropHandle (HDROP handle);
	};

	// Responsible for freeing drop resources
	class FileDropOwner : public FileDropHandle
	{
	public:
		FileDropOwner (WPARAM wParam)
			: FileDropHandle (reinterpret_cast<HDROP> (wParam))
		{}
		~FileDropOwner ()
		{
			::DragFinish (ToNative ());
		}
	};

	// Does the work of dragging
	class FileDragger
	{
	public:
		FileDragger::FileDragger (std::vector<std::string> const & files,
									bool isRightButtonDrag);
		void Do ();
	private:
		Com::IfacePtr<IDataObject>	_fileDrop;
		Com::IfacePtr<IDropSource>	_dropSource;
		unsigned long				_dropResult;
	};

	// Derive from this interface to receive drag&drop notifications from Windows Shell
	class FileDropSink
	{
	public:
		FileDropSink();
		void RegisterAsDropTarget (Win::Dow::Handle win);
		void virtual OnDragEnter (Win::Point dropPoint) {}
		void virtual OnDragLeave () {}
		void virtual OnDragOver (Win::Point dropPoint) {}
		void virtual OnFileDrop (Win::FileDropHandle droppedFiles, Win::Point dropPoint) = 0;

	private:
		Com::IfacePtr<IDropTarget>	_dropTarget;

	private:
		// Handles interaction with Windows Shell. Supports only Clipboard::FileDrop format
		class Target : public IDropTarget
		{
		public:
			Target (Win::FileDropSink & dropSink)
				: _refCount (1),
				  _dropSink (dropSink)
			{}

			// IUnknown interface
			STDMETHODIMP QueryInterface (REFIID ifaceId, void ** ifacePtr);
			STDMETHODIMP_(ULONG) AddRef () { return ++_refCount; }
			STDMETHODIMP_(ULONG) Release ();

			// IDropTarget interface
			STDMETHODIMP DragEnter (IDataObject * pDataObject,	// Pointer to the interface of the source data object
									DWORD grfKeyState,			// Current state of keyboard modifier keys
									POINTL pt,					// Current cursor coordinates (in screen coordinates)
									DWORD * pdwEffect);			// Pointer to the effect of the drag-and-drop operation
			STDMETHODIMP DragLeave (void);
			STDMETHODIMP DragOver (DWORD grfKeyState,		// Current state of keyboard modifier keys
								   POINTL pt,				// Current cursor coordinates (in screen coordinates)
								   DWORD * pdwEffect);		// Pointer to the effect of the drag-and-drop operation
			STDMETHODIMP Drop (IDataObject * pDataObject,	// Pointer to the interface for the source data
							   DWORD grfKeyState,			// Current state of keyboard modifier keys
							   POINTL pt,					// Current cursor coordinates (in screen coordinates)
							   DWORD * pdwEffect);			// Pointer to the effect of the drag-and-drop operation

		private:
			unsigned long		_refCount;
			Win::FileDropSink &	_dropSink;
		};
	};
}

#endif
