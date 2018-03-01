#if !defined (Session_H)
#define Session_H
// Reliable Software (c) 2002
#include "DataPortal.h"
#include "Lister.h"
#include <File/Path.h>
#include <Sys/Active.h>
#include <File/Dir.h>
namespace XML { class Tree; }

// Purpose: keeps control state for the UI (where are we in the hierarchy?)
// Schedules work to be done

class Session: public Data::Sink
{
public:
	Session (Data::Sink & sink, int id) 
		:_sink (sink),
		 _id (id)
	{}
	int GetId () const { return _id; }
	virtual ~Session ();
	virtual std::string GetCaption () const = 0;
	virtual std::string GetDir () const = 0;
	virtual bool IsDiff () const { return false; }
	virtual bool IsDown () const = 0;
	virtual void DirUp () = 0;
	virtual void DirDown (std::string const & name, DiffState state) = 0;
	virtual void OpenFile (std::string const & name, DiffState state) = 0;
	virtual void Refresh (bool restart) = 0;

	void QueryDone ();
	// Data::Sink
	void DataReady (Data::ChunkHolder data, bool isDone, int srcId);
protected:
	void CallDiffer (XML::Tree const & args);
protected:
	int			_id;
	Data::Sink &_sink;
	// worker thread
	auto_active<Data::Provider> _worker;
};

class SessionDir: public Session
{
public:
	SessionDir (Data::Sink & sink, int id)
		: Session (sink, id)
	{}
	void SetDir (std::string const & dir) { _path.Change (dir); }
	std::string GetDir () const { return _path.ToString (); }
	std::string GetCaption () const { return _path.ToString (); }

	bool IsDown () const { return !_path.IsDirStrEmpty (); }
	void DirUp ();
	void DirDown (std::string const & name, DiffState state);
	void OpenFile (std::string const & name, DiffState state);
	void Refresh (bool restart);
private:
	FilePath	_path;
};

class SessionDiff: public Session
{
public:
	SessionDiff (Data::Sink & sink, int id)
		: Session (sink, id),
		  _dirDepth1 (0),
		  _dirDepth2 (0)
	{}
	void SetOldDir (std::string const & dir) 
	{
		_dirDepth1 = 0;
		_path1.Change (dir);
	}
	void SetNewDir (std::string const & dir)
	{ 
		_dirDepth2 = 0;
		_path2.Change (dir); 
	}
	std::string GetOldDir () const { return _path1.ToString (); }
	std::string GetNewDir () const { return _path2.ToString (); }
	std::string GetDir () const { Assert (!"GetDir called"); return std::string ("Error!"); }
	std::string GetCaption () const;

	bool IsDown () const { return _dirDepth1 > 0 || _dirDepth2 > 0; }
	bool IsDiff () const; 
	void DirUp ();
	void DirDown (std::string const & name, DiffState state);
	void OpenFile (std::string const & name, DiffState state);
	void Refresh (bool restart);
private:
	FilePath	_path1;
	int			_dirDepth1; // when going inside sub-folders
	FilePath	_path2;
	int			_dirDepth2;
};

#endif
