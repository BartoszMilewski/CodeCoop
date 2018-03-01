#if !defined (FINDDLG_H)
#define FINDDLG_H
//------------------------------------
//  (c) Reliable Software, 2000 - 2005
//------------------------------------

#include "Search.h"
#include "Resource.h"

#include <Win/Dialog.h>
#include <Ctrl/Button.h>
#include <Ctrl/ComboBox.h>
#include <Ctrl/Accelerator.h>

// only one copy of SearchRequest in derived classes

class FindDialogData : public virtual SearchRequest
{
public :
	FindDialogData ()
		: _dialogWasOpen (false), _canEdit (true)
	{}

	// find options	
	void SetMatchCase (bool matchCase) { _matchCase = matchCase; }
	void SetWholeWord (bool wholeWord) { _wholeWord = wholeWord; }
	bool SetCanEdit (bool canEdit); // return true if _canEdit is changed
	bool CanEdit () const { return _canEdit; }
	// dialog positions
	void SetPaneMode (bool isOnePane) { _isOnePane = isOnePane; }
	void RememberDlgPosition (int x, int y);
	// return true if dialog was open during this session
	bool GetDlgPosition (int & x, int & y);
	void RememberDlgPositions (int xLeft1, int yTop1, int xLeft2, int yTop2);
	void Activate () {_dialogWasOpen = true;}
	// handles
	Dialog::Handle GetDialog () { return _hDialog; }
	void SetDialog (Dialog::Handle  win) { _hDialog = win; }
	void ResetDialog () { _hDialog.Reset (); }
	bool IsOpenDialog () { return !_hDialog.IsNull (); }

protected:	
	class DialogPosition 
	{
	public :
		DialogPosition (int xLeft = 0, int yTop = 0)
			: _xLeft (xLeft),
			  _yTop (yTop)
		{}
		int GetXLeft () { return _xLeft;}
		int GetYTop () { return _yTop;}
		void RememberXLeft (int xLeft) { _xLeft = xLeft; } 
		void RememberYTop (int yTop) { _yTop = yTop; }

	private :
		int _xLeft;
		int _yTop;
	};

	Dialog::Handle			_hDialog;
	bool                    _isOnePane;
	DialogPosition          _onePanePosition;
	DialogPosition          _twoPanePosition;
	bool                    _dialogWasOpen;
	bool                    _canEdit;
};

class ReplaceDialogData : public FindDialogData, public ReplaceRequest
{
public :
	ReplaceDialogData ()
		:_hintOn (true)
	{}
	bool IsHintOn () { return _hintOn;}
	void ToggleHint (bool hintOn) { _hintOn = hintOn; }
	void SetSubstitution (std::string & s) { _substitution = s; }
	bool GetAssociation (const std::string & key, std::string & value);
	void SetAssociation (const std::string & key, const std::string & value);
	std::list <std::string> & AssocKeyList () { return _assotiations.GetKeyList ();}
	std::list <std::string> & AssocValueList () { return _assotiations.GetValueList ();}
	void ClearAssociations () {_assotiations.Clear ();}
private:
	// remembers what was replaced with what
	class Associations
	{
	public:
		bool GetAssociation (const std::string & key, std::string & value);
		void SetAssociation (const std::string & key, const std::string & value);
		std::list <std::string> & GetKeyList () { return _keyList;}
		std::list <std::string> & GetValueList () { return _valueList;}
		void Clear ()  
		{
			_keyList.clear ();
			_valueList.clear ();
		}	   
	private:
		std::list <std::string> _keyList;
	    std::list <std::string> _valueList;
	};

	Associations            _assotiations;
	bool                    _hintOn;
};

class ComboBoxNotificationSink
{
public:
	virtual ~ComboBoxNotificationSink () {}

	virtual void OnEditChange (int idBox) {}
	virtual void OnSelChange (int idBox) {}
};

class FindHandler : public Win::ComboBox::NotifyHandler
{
public:
	FindHandler (ComboBoxNotificationSink * dlgCtrl, int idBox)
		: _dlgCtrl (dlgCtrl), _idBox (idBox)
	{}		
	void OnEditChange () {_dlgCtrl->OnEditChange (_idBox);}			
	void OnSelChange () {_dlgCtrl->OnSelChange(_idBox);}
private:
	ComboBoxNotificationSink * _dlgCtrl;
	int                        _idBox;
};

class FindDialogCtrl : public Dialog::ControlHandler, public ComboBoxNotificationSink
{
public:
    FindDialogCtrl (FindDialogData & dlgData, int dlgId);

    bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();
	void OnDestroy () throw ();
	void OnActivate () throw (Win::Exception);
	bool OnCommand (int cmdId, bool isAccel) throw ();
	void SetEditStatus ();

protected:
	virtual void CloseDialog () throw ();
	void SetDialogState ();
	void RetriveStringList (std::list<std::string> & list, Win::ComboBox & box);
	void FillListBox (std::list <std::string> & list, Win::ComboBox & box);
	void RefreshList (Win::ComboBox & box, const std::string & newItem);
protected:
	enum { maxSizeEditList = 10};

	Win::Button		   		_findNext;
	Win::Button		   		_findReplace;
	Win::Button        		_close;
	Win::ComboBox	  		_findCombo;
	Win::RadioButton   		_up;
	Win::RadioButton   		_down;
	Win::CheckBox      		_matchWholeWord;
	Win::CheckBox      		_matchCase;
	FindDialogData  &  		_dlgData;
};

class ReplaceDialogCtrl : public FindDialogCtrl
{
public :
	ReplaceDialogCtrl (ReplaceDialogData & dlgData, int dlgId);

	bool OnInitDialog () throw (Win::Exception); 
	bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	void OnDestroy () throw  ();
	void OnActivate () throw (Win::Exception);
	
	// ComboBoxNotificationSink--receives notification from _replaceCombo
	virtual void OnEditChange (int idBox);
	virtual void OnSelChange (int idBox);
	void SetEditStatus ();
	
private :
	void SetDialogState ();
	bool ShowHint (); // return true if is association
	void CloseDialog () throw ();

	Win::Button		   		_replace;
	Win::Button		   		_replaceAll;
	Win::ComboBox	   		_replaceCombo;
	Win::CheckBox      		_hint;
	ReplaceDialogData & 	_replaceDlgData;
};

// This object has a lifetime longer than the lifetime of the modeless dialogs it displays
class FindPrompter
{
public:
	FindPrompter (Win::MessagePrepro & msgPrepro, Win::Dow::Handle parentWin, Accel::Handle accel);
	void InitDialogFind (Win::Dow::Handle & focusWin);
	void InitDialogReplace (Win::Dow::Handle & focusWin);
	void AttachToWindow (Win::Dow::Handle parentWin);
	void AttachToKbdAccelerator (Accel::Handle dlgKbdAccel);
	void SetPaneMode (bool isOnePane);
	void RememberDlgPositions (int xLeft1, int yTop1, int xLeft2, int yTop2);
	bool GetDlgPosition (int & xLeft, int & yTop);
	void FindNext (bool directionBackward = false);
	void SetCanEdit (bool canEdit);
private:
	void InitDialog (Win::Dow::Handle & focusWin, int idDialog);
	Win::MessagePrepro &	_msgPrepro;
	ReplaceDialogData      	_commonData;
	FindDialogCtrl			_findCtrl;
	ReplaceDialogCtrl		_replaceCtrl;
	Win::Dow::Handle		_parentWin;
	Accel::Handle			_accel;
};
#endif
