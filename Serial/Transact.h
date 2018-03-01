#if !defined TRANSACT_H
#define TRANSACT_H
//---------------------------------------------------
//  transact.h
//  (c) Reliable Software, 1996, 97, 98, 99, 2000, 01
//---------------------------------------------------

#include "Serialize.h"
#include "Transactable.h"

class SysPathFinder;
namespace Win { class ExitException; }

class SwitchFile : public File
{
    enum DataState
    {
        // Some arbitrary bit patterns
        dataFileOneValid = 0xc6,
        dataFileTwoValid = 0x3A,
        badDataState = 0xff
    };

	enum TransPhase
	{
		phaseRedo = 0xd7,
		phaseUndo = 0x6A
	};

public:
    SwitchFile ();
    SwitchFile (SysPathFinder const & pathFinder);
    
    void Open (char const *fName);

    bool IsDataOneValid () const throw () { return _dataState == dataFileOneValid; }
	bool MustRedo () const throw () { return _phase == phaseRedo; }

    void MakeDataOneValid () throw () { _dataState = dataFileOneValid; }
    void MakeDataTwoValid () throw () { _dataState = dataFileTwoValid; }
	void SetRedo () throw () { _phase = phaseRedo; }
	void SetUndo () throw () { _phase = phaseUndo; }
    void FlushState ();
    void FlushStateOrDie (); //  throw (Win::ExitException);

private:
    DataState		_dataState;
	TransPhase		_phase;
};

class ClearTransaction
{
public:
    ClearTransaction (Transactable& xable);
	~ClearTransaction ();
	void Commit () throw ();

protected:
    Transactable&		_xable;
    bool				_commit;
};

class ReadTransaction : public ClearTransaction
{
public:
    ReadTransaction (Transactable& xable, SysPathFinder& pathFinder);
    Deserializer& GetDeserializer () throw () { return _data; }

private:
    FileDeserializer	_data;
    SwitchFile			_switch;
};

class Transaction
{
public:
    Transaction (Transactable& xable, SysPathFinder& pathFinder);
    ~Transaction ();
    void Commit ();
    void CommitAndDelete (SysPathFinder& pathFinder);
    FileSerializer& GetSerializer () { return _backup; }
    Deserializer& GetDeserializer () throw () { return _data; }
protected:
	void CommitPhaseOne ();
	void CommitPhaseTwo (); //  throw (Win::ExitException);
private:
    Transactable&		_xable;
    bool				_commit;
    FileDeserializer	_data;
    SwitchFile			_switch;

    bool            _delete;
    FileSerializer  _backup;
};

class StagingCleanup
{
public:
	StagingCleanup (SysPathFinder & sysPathFinder);
	bool MustRedo ();
	void Commit ();
private:
    SwitchFile      _switch;
};

#endif
