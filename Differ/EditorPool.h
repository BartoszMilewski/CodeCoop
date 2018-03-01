#if !defined (EDITORPOOL_H)
#define EDITORPOOL_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------
#include "EditCtrl.h"
#include "FileViews.h"
namespace Win { class Message; }
namespace Font { class Maker; }
class EditBuf;

class DocEditor
{
public:
	DocEditor (FileSelection tag, Win::Dow::Handle parentWin, unsigned childId);
	void SetTag (FileSelection tag) { _tag = tag; }
	FileSelection GetTag () const { return _tag; }
	Win::Dow::Handle GetWindow () const { return _win; }
	void MakeEditable (bool val) { _ctrl.MakeEditable (val); }
private:
	Win::Dow::Handle	_win;
	EditController		_ctrl;
	FileSelection		_tag;
};

class EditorPool
{
public:
	EditorPool ();
	void Init (Win::Dow::Handle parentWin);
	Win::Dow::Handle GetDiffWindow () const { return _diffEditor->GetWindow (); }
	Win::Dow::Handle GetEditableWindow () const;
	void SetEditableReadOnly (bool isReadOnly);
	void SwitchWindow (FileSelection sel);
	void InitDocument (FileSelection sel, std::unique_ptr<EditBuf> editBuf);
	void AddDocument (FileSelection sel, std::unique_ptr<EditBuf> editBuf);
	DocEditor * FindDocEditor (FileSelection sel) const;
	void RemoveDocument (FileSelection sel);
	bool IsLineBreakingOn () const { return _lineBreakingOn; }
	void SetLineBreaking (bool on);
	void ToggleLineBreaking ();
	void FontChange (unsigned tabSize, Font::Maker const & fontMaker);
	void SetTabSize (unsigned tabSize);
	void SendEditorMsg (Win::Message & msg)
	{
		SendEditorMsgExcept (msg, Win::Dow::Handle ());
	}
	void SendEditorMsgExcept (Win::Message & msg, Win::Dow::Handle exceptWin);
private:
	Win::Dow::Handle	_parentWin;
	bool                _lineBreakingOn;
	std::unique_ptr<DocEditor>	_diffEditor;
	auto_vector<DocEditor>		_editors;
};

#endif
