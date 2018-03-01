#if !defined (PROMPTER_H)
#define PROMPTER_H
//----------------------------------
// (c) Reliable Software 1998 - 2008
//----------------------------------

#include "HelpEngine.h"
#include <StringOp.h>

class InputSource;
class JoinProjectData;
class FileRequest;

namespace Win
{
	class CritSection;
}

namespace Dialog
{
	class ControlHandler;
}

namespace PropPage
{
	class HandlerSet;
}

class Extractor
{
public:
	virtual ~Extractor () {}
	virtual bool GetDataFrom (NamedValues const & namedValues) = 0;
};

class StringExtractor: public Extractor
{
public:
	StringExtractor (std::string const & name)
		: _name (name)
	{}

	bool GetDataFrom (NamedValues const & namedValues)
	{
		_value = namedValues.GetValue (_name);
		return !_value.empty ();
	}
	std::string const & GetString () const
	{
		return _value;
	}

private:
	std::string _name;
	std::string _value;
};

class NumberExtractor: public Extractor
{
public:
	NumberExtractor (std::string const & name)
		: _name (name), _value(-1)
	{}

	bool GetDataFrom (NamedValues const & namedValues)
	{
		std::string strVal = namedValues.GetValue (_name);
		if (strVal.empty ())
			return false;
		_value = ToInt(strVal);
		return true;
	}
	int GetNumber() const
	{
		return _value;
	}

private:
	std::string _name;
	int _value;
};

class WindowExtractor: public Extractor
{
public:
	WindowExtractor (std::string const & name)
		: _name (name), _value(Win::Dow::Handle())
	{}

	bool GetDataFrom (NamedValues const & namedValues)
	{
		std::string strVal = namedValues.GetValue (_name);
		if (strVal.empty ())
			return false;
		_value = ToHwnd(strVal);
		return true;
	}
	Win::Dow::Handle GetHandle() const
	{
		return _value;
	}

private:
	std::string _name;
	Win::Dow::Handle _value;
};

class JoinDataExtractor : public Extractor
{
public:
	JoinDataExtractor (JoinProjectData & data)
		: _data (data)
	{}

	bool GetDataFrom (NamedValues const & namedValues);

private:
	JoinProjectData & _data;
};

class BackupRequestExtractor : public Extractor
{
public:
	BackupRequestExtractor (FileRequest & data)
		: _data (data)
	{}

	bool GetDataFrom (NamedValues const & namedValues);

private:
	FileRequest & _data;
};

class Prompter
{
public:
	Prompter ()
		: _win (0),
		  _critSect (0)
	{}

	void SetWindow (Win::Dow::Handle win) {	_win = win;	}
	void AddUncriticalSect (Win::CritSection & critSect) { _critSect = &critSect; }

	bool HasNamedValues () const { return _argCache.size () != 0; }
	void SetNamedValue(std::string const & name, Win::Dow::Handle h)
	{
		Assert (sizeof(h.ToNative()) == sizeof(int));
		_argCache.Add(name, ToString(reinterpret_cast<int>(h.ToNative())));
	}
	void SetNamedValue (std::string const & name, std::string const & value)
	{
		_argCache.Add (name, value);
	}
	void SetNamedValue (std::string const & name, int value)
	{
		_argCache.Add (name, ToString(value));
	}
	void ClearNamedValues ()
	{
		_argCache.Clear ();
	}

	bool GetData (Dialog::ControlHandler & ctrl, InputSource * alternativeInput = 0);
	bool GetData (PropPage::HandlerSet & ctrlSet, InputSource * alternativeInput = 0);
	bool GetData (Extractor & extr, InputSource * alternativeInput = 0);

private:
	Win::Dow::Handle	_win;
	HelpEngine			_helpEngine;
	Win::CritSection *	_critSect;
	NamedValues			_argCache;
};

//----------------
// Global prompter
//----------------

extern Prompter ThePrompter;

#endif
