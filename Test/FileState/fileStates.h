#if !defined (FILESTATES_H)
#define FILESTATES_H
// ---------------------------
// (c) Reliable Software, 2004
// ---------------------------
#include <StringOp.h>

class FileStateMap
{
public:
	FileStateMap ();
	bool GetState (std::string const & stateStr,
				   unsigned long & state,
				   unsigned long & unsignificantBitsMask);
private:
	class State
	{
	public:
		unsigned long _value;
		unsigned long _mask; // some bits may have arbitrary values in a specific state
	};
	class Name2State
	{
	public:
		char const *  _name;
		State		  _state;
	};

	static const Name2State _stateTable [];

private:
	NocaseMap<State> _stateMap;
};

#endif
