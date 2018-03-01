#if !defined (EDITPANE_H)
#define EDITPANE_H
//
// (c) Reliable Software, 1997-2006
//

#include "Document.h"
#include "Select.h"
#include "Mapper.h"
#include "ViewPort.h"
#include "Caret.h"
#include "PaintContext.h"
#include "EditLog.h"
#include "Search.h"

#include <Sys/Timer.h>
#include <Win/Win.h>
#include <Win/Controller.h>
#include <Win/Keyboard.h>

class EditBuf;
class EditPaneController;

class KbdHandler : public Keyboard::Handler
{
public:
	KbdHandler (EditPaneController * ctrl)
		: _ctrl (ctrl)
	{}

	bool OnPageUp () throw ();
	bool OnPageDown () throw ();
	bool OnUp () throw (); 
	bool OnDown () throw (); 
	bool OnLeft () throw (); 
	bool OnRight () throw ();
	bool OnHome () throw ();
	bool OnEnd () throw (); 
	bool OnDelete () throw ();

private:
	EditPaneController * _ctrl;
};

class LineNotificationSink
{
public:
	virtual ~LineNotificationSink () {}

	virtual void InvalidateLines (int lineBegin, int lineCunt) = 0;
	virtual void ShiftLines (int lineBegin, int shift) = 0;
	virtual void UpdateScrollBars () = 0;
	virtual void RefreshLineSegment (int docPara, int offsetBegin, int offsetEnd) = 0;
};

class EditStateNotificationSink
{
public :
	EditStateNotificationSink ()
		:_actionCounter (0)
	{}
	virtual ~EditStateNotificationSink () {}

	void ForwardAction (); 
	void BackwardAction ();
	virtual void OnChangeState () = 0;
protected:
	void OnSavedDoc ();
	bool IsSavedDoc () const { return _actionCounter == 0;}
protected:
	int  _actionCounter; // It count from the last saved action .The _counter my be negative number.
};

class EditPaneController : public Win::Controller, public LineNotificationSink, public EditStateNotificationSink
{
	friend class KbdHandler;

public:
	EditPaneController ();
	~EditPaneController ();

	Keyboard::Handler * GetKeyboardHandler () throw () { return &_kbdHandler; }
	bool OnCreate (Win::CreateData const * create, bool & success) throw ();
	bool OnFocus (Win::Dow::Handle winPrev) throw ();
	bool OnKillFocus (Win::Dow::Handle winNext) throw ();
	bool OnSize (int width, int height, int flag) throw ();
	bool OnPaint () throw ();

	bool OnCommand (int cmdId, bool isAccel) throw ();
	bool OnTimer (int id) throw ();
	// Mouse
	bool OnMouseMove (int x, int y, Win::KeyState kState) throw ();
	bool OnLButtonDown (int x, int y, Win::KeyState kState) throw ();
	bool OnLButtonUp (int x, int y, Win::KeyState kState) throw ();
	bool OnLButtonDblClick (int x, int y, Win::KeyState kState) throw ();
	bool OnCaptureChanged (Win::Dow::Handle newCaptureWin) throw ();
	// Keyboard
	bool OnChar (int vKey, int flags) throw ();
	bool OnUserMessage (Win::UserMessage & msg) throw ();

private:
	void InitDoc (std::unique_ptr<EditBuf> editBuf);
	void NewParagraph ();
	int  GetDocDimension (int bar);
	int  GetWinDimension (int bar);
	int  GetScrollPosition (int bar);
	void SelectAll ();
	void SetDocPosition (int bar, int pos, bool notify);
	int  GetDocPosition (int bar);
	void SynchScrollToDocPos (int offset, int targetPara);
	void ScrollToFullView (int docLine);
	void ScrollIfNeeded (int paraNo, int docColBegin, int docColEnd);
	void StartSelectLine (int flags, int y);
	void ContinueSelectLine (int y);
	void EndSelectLine (int y);
	void CopySelectionToClipboard ();
	void CopySelectionToBuf (std::string & out);
	bool Paste ();
	void Cut ();
	void Delete ();
	void LButtonDrag (int x, int y);
	int  AutoIndent (int paraNo);

	void NextChange ();
	void PrevChange ();
	void GetSelection (SearchRequest * data);
	bool FindNext (SearchRequest const * data);
	void Replace (ReplaceRequest const * data);
	void ReplaceAll (ReplaceRequest const * data);
	bool EqualsSelection (Selection::ParaSegment const & segment);
	bool IsSelectedForReplace (SearchRequest const * data);//only finding word is selected (match wich
	                                                // user preference)
	void ChangeFont (unsigned tabSize, Font::Maker * newFont);

	LRESULT DoesNeedSave (unsigned int * newSize, bool getSizeAlways) const;
	LRESULT IsReadOnly () const;
	LRESULT GetBuf (char * buf, bool willBeSaved);

	void SelectWord (int col, int paraNo);
	void ClearSelection ();
	void ModifySelection (bool isShift);
    void AdjustSelection (int docColPos, int docParaPos, int prevDocColPos, int prevDocParaPos);
	void ScrollToDocPos ();
	void DocPosNotify () const;
	// Navigation
	void SetPositionAdjust (int & visCol, int visLine, int & docCol, int & docPara);
	void PositionAdjust (int & visCol, int visLine, int & docCol, int & docPara);
	void SetDocPositionShow (DocPosition  docPos);
	void Up (bool isShift, bool isCtrl);
	void Down (bool isShift, bool isCtrl);
	void Left (bool isShift, bool isCtrl);
	void Right (bool isShift, bool isCtrl);
	void PageDown (bool isShift);
	void PageUp (bool isShift);
	void Home (bool isShift, bool isCtrl);
	void End (bool isShift, bool isCtrl);
	void Escape ();
	// Editing
	bool CanEdit ();
	void BackSpace ();
	void DeleteSelection ();
	void MergeParagraphs (int startPara);
	void ToggleLineBreaking (bool lineBreakingOn);
	void SetCaret ();
	void GoToParagraph (int paraNo);
	void ClearEditState ();
	void ChangeReadOnlyState (bool readOnly);
	void Undo (int numberAction);
	void Redo (int numberAction);
	bool MultiParaTab (bool isShift); // return true if the tabs have been inserted (or deleted)

	//LineNotificationSink :
	void InvalidateLines (int startVisLine, int count);
    void ShiftLines (int startVisLine, int shift);
	void UpdateScrollBars ();
	void RefreshLineSegment (int docPara, int offsetBegin, int offsetEnd);
	// EditStateNotificationSink
	void OnChangeState ();
	EditLog * SwitchLog (EditLog * newLog);

private:
	Win::Dow::Handle	_hwndParent;
	KbdHandler			_kbdHandler;
	int					_cx;
	int					_cy;

	std::unique_ptr<FontBox>	_fontBox;

	// Drag support
	POINTS						_mousePoint;
	Win::Timer					_timer;
	std::unique_ptr<ColMapper>	_mapper; // after _fontBox is created
	std::unique_ptr<ViewPort>		_viewPort;
	Selection::Dynamic			_selection;
	std::unique_ptr<Caret>		_caret;
	RedoLog						_redoLog;
	UndoLog						_undoLog;
	Document					_doc;
	bool						_isReadOnly;	
};

#endif
