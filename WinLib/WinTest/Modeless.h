#if !defined (MODELESS_H)
#define MODELESS_H
//-------------------------------
// (c) Reliable Software, 2006
//-------------------------------
#include <Win/Dialog.h>
#include <Ctrl/Button.h>

class ModelessManager
{
public:
	ModelessManager (Win::MessagePrepro & msgPrepro) 
		: _msgPrepro (msgPrepro), _flag (false) {}
	void Open (Win::Dow::Handle parentWin);
	bool IsActive () const { return !_win.IsNull (); }
	void Deactivate () { _win.Reset (); }
	void SetFlag (bool flag) { _flag = flag; }
	bool IsFlag () const { return _flag; }
private:
	Dialog::Handle		_win;
	Win::MessagePrepro & _msgPrepro;
	bool _flag;
	std::unique_ptr<Dialog::ControlHandler> _handler;
};

class ModelessHandler: public Dialog::ControlHandler
{
public:
	ModelessHandler (ModelessManager & man);
    bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	void OnDestroy () throw ()
	{
		_man.Deactivate ();
	}

private:
	Win::CheckBox _check;
	ModelessManager & _man;
};

#endif
