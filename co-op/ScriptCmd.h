#if !defined (SCRIPTCMD42_H)
#define SCRIPTCMD42_H
//------------------------------------
//  (c) Reliable Software, 1997 - 2007
//------------------------------------

#include "Serialize.h"

#include <Dbg/Assert.h>

#include <iosfwd>

namespace Version40
{
	enum ControlType
	{
		typeNone = 0,
		typeAck = 1,
		typeNack = 2,				// Obsolete -- never used
		typeMakeReference = 3,
		typeDefect = 4,				// Obsolete -- used in version < 2.0; replaced by typeMembershipUpdate
		typeJoinRequest = 5, 
		typeNewUser = 6,
		typeAckUserInfo = 7,
		typeMembershipUpdate = 8
	};
}

enum ScriptCmdType
{
	// These command types are identical to version 4.2
	typeWholeFile = 0,
	typeTextDiffFile = 1,
	typeDeletedFile = 2,
	typeNewFolder = 3,
	typeDeleteFolder = 4,
	typeUserCmd = 5,	// Obsolete in version 4.5
	typeBinDiffFile = 6,
	// These are new command types introduced in version 4.5
    typeAck = 7,
    typeMakeReference = 8,
    typeResendRequest = 9,
    typeNewMember = 10,
	typeDeleteMember = 11,
	typeEditMember = 12,
    typeJoinRequest = 13,
	typeResendFullSynchRequest = 14,
	// These are new command types introduced in version 5.1
	typeVerificationRequest = 15
};

std::ostream & operator<<(std::ostream & os, ScriptCmdType type);

class DumpWindow;
class PathFinder;

class ScriptCmd : public Serializable
{
public:
	static std::unique_ptr<ScriptCmd> DeserializeCmd (Deserializer & in, int version);
	static std::unique_ptr<ScriptCmd> DeserializeV40CtrlCmd (Deserializer & in, int version);

	virtual ~ScriptCmd () {}

	virtual ScriptCmdType GetType () const = 0;
	virtual void Dump (DumpWindow & dumpWin, PathFinder const & pathFinder) const
	{
		Assert (!"Illegal call to ScriptCmd::Dump");
	}
	virtual void Dump (std::ostream & os) const
	{
		// Default diagnostic dump implementation does nothing.
	}
	virtual bool Verify () const { return true; }
	virtual bool IsEqual (ScriptCmd const & cmd) const { return false; }

	// Serializable interface
	virtual void Serialize (Serializer& out) const
	{
		Assert (!"Illegal call to ScriptCmd::Serialize");
	}
	virtual void Deserialize (Deserializer& in, int version)
	{
		Assert (!"Illegal call to ScriptCmd::Deserialize");
	}
};

#endif
