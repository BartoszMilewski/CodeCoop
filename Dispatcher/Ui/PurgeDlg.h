#if !defined (PURGEDLG_H)
#define PURGEDLG_H
// ----------------------------------
// (c) Reliable Software, 2001 - 2005
// ----------------------------------

#include <Win/Dialog.h>
#include <Ctrl/Button.h>

class PurgeData
{
public:
	PurgeData () : _purgeLocal (true), _purgeSat (false) {}
	
	void SetPurgeLocal (bool purge) { _purgeLocal = purge; }
	void SetPurgeSatellite (bool purge) { _purgeSat = purge; }
	
	bool IsPurgeLocal () const { return _purgeLocal; }
	bool IsPurgeSatellite () const { return _purgeSat; }

private:
	bool _purgeLocal;
	bool _purgeSat;
};

class PurgeCtrl : public Dialog::ControlHandler
{
public:
    PurgeCtrl (PurgeData & data);

    bool OnInitDialog () throw (Win::Exception);
    bool OnApply () throw ();

private:
	PurgeData & _data;

	Win::CheckBox	_local;
	Win::CheckBox	_sat;
};

#endif
