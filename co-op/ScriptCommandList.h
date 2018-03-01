#if !defined (SCRIPTCOMMANDLIST_H)
#define SCRIPTCOMMANDLIST_H
//------------------------------------
//  (c) Reliable Software, 2003 - 2007
//------------------------------------

#include "ScriptSerialize.h"
#include "ScriptCommands.h"
#include "Params.h"

#include <auto_vector.h>

class CommandList : public ScriptSerializable
{
public:
	CommandList () {}
    CommandList (Deserializer& in)
	{
		Read (in);
	}

	void push_back (std::unique_ptr<ScriptCmd> cmd) { _cmds.push_back (std::move(cmd)); }

	unsigned int size () const { return _cmds.size (); }
	void clear () { _cmds.clear (); }
	FileCmd const * front () const { return dynamic_cast<FileCmd const *>(_cmds.front ()); }
	FileCmd const * back () const { return dynamic_cast<FileCmd const *>(_cmds.back ()); }
	void swap (CommandList & cmdList) { _cmds.swap (cmdList._cmds); }
	bool IsEqual (CommandList const & cmds) const;

    bool IsSection () const { return true; }
    int  SectionId () const { return 'SCMD'; }
    int  VersionNo () const { return scriptVersion; }
	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);

public:
	class Sequencer
	{
	public:
		Sequencer (CommandList const & list)
			: _cur (list._cmds.begin ()),
			  _end (list._cmds.end ())
		{}

		bool AtEnd () const { return _cur == _end; }
		void Advance () { ++_cur; }

		ScriptCmd const & GetCmd () const { return **_cur; }
		
		FileCmd const & GetFileCmd () const 
			{ return dynamic_cast<FileCmd const &>(GetCmd ()); }
		CtrlCmd const & GetCtrlCmd () const 
			{ return dynamic_cast<CtrlCmd const &>(GetCmd ()); }
		MemberCmd const & GetMemberCmd () const 
			{ return dynamic_cast<MemberCmd const &>(GetCmd ()); }
		JoinRequestCmd const & GetJoinRequestCmd () const 
			{ return dynamic_cast<JoinRequestCmd const &>(GetCmd ()); }
		ResendFullSynchRequestCmd const & GetFsResendCmd () const 
			{ return dynamic_cast<ResendFullSynchRequestCmd const &> (GetCmd ()); }
		VerificationRequestCmd const & GetVerificationRequestCmd () const 
			{ return dynamic_cast<VerificationRequestCmd const & > (GetCmd ()); }
		NewMemberCmd const & GetAddMemberCmd () const 
			{ return dynamic_cast<NewMemberCmd const &>(GetCmd ()); }
		DeleteMemberCmd const & GetDeleteMemberCmd () const 
			{ return dynamic_cast<DeleteMemberCmd const &>(GetCmd ()); }
		EditMemberCmd const & GetEditMemberCmd () const 
			{ return dynamic_cast<EditMemberCmd const &>(GetCmd ()); }

	private:
		auto_vector<ScriptCmd>::const_iterator	_cur;
		auto_vector<ScriptCmd>::const_iterator	_end;
	};

	class EditSequencer
	{
	public:
		EditSequencer (CommandList & list)
			: _cur (list._cmds.begin ()),
			  _end (list._cmds.end ())
		{}
		EditSequencer ()
		{}
		void Restart (CommandList & newList)
		{
			_cur = newList._cmds.begin ();
			_end = newList._cmds.end ();
		}

		bool AtEnd () const { return _cur == _end; }
		void Advance () { ++_cur; }

		ScriptCmd & GetCmd () const { return **_cur; }
		FileCmd & GetFileCmd () const { return dynamic_cast<FileCmd &>(GetCmd ()); }
		CtrlCmd & GetCtrlCmd () const { return dynamic_cast<CtrlCmd &>(GetCmd ()); }
		MemberCmd & GetMemberCmd () const { return dynamic_cast<MemberCmd &>(GetCmd ()); }
		JoinRequestCmd & GetJoinRequestCmd () const { return dynamic_cast<JoinRequestCmd &>(GetCmd ()); }
		NewMemberCmd & GetAddMemberCmd () const { return dynamic_cast<NewMemberCmd &>(GetCmd ()); }
		DeleteMemberCmd & GetDeleteMemberCmd () const { return dynamic_cast<DeleteMemberCmd &>(GetCmd ()); }
		EditMemberCmd & GetEditMemberCmd () const { return dynamic_cast<EditMemberCmd &>(GetCmd ()); }

	private:
		auto_vector<ScriptCmd>::iterator	_cur;
		auto_vector<ScriptCmd>::iterator	_end;
	};

	class ReverseSequencer
	{
	public:
		ReverseSequencer (CommandList const & list)
			: _cur (list._cmds.rbegin ()),
			  _end (list._cmds.rend ())
		{}

		bool AtEnd () const { return _cur == _end; }
		void Advance () { ++_cur; }

		ScriptCmd const & GetCmd () const { return **_cur; }
		FileCmd const & GetFileCmd () const { return dynamic_cast<FileCmd const &>(GetCmd ()); }

	private:
		auto_vector<ScriptCmd>::const_reverse_iterator	_cur;
		auto_vector<ScriptCmd>::const_reverse_iterator	_end;
	};

	friend class Sequencer;
	friend class EditSequencer;
	friend class ReverseSequencer;

protected:
	auto_vector<ScriptCmd>	_cmds;
};

#endif
