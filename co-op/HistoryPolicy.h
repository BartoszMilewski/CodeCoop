#if !defined (HISTORYPOLICY_H)
#define HISTORYPOLICY_H
//---------------------------------------------
// (c) Reliable Software 2005
//---------------------------------------------

#include "GlobalId.h"

namespace Project
{
	class Db;
}

namespace History
{
	class Node;

	class Policy
	{
	public:
		Policy () {}

		virtual bool XCanRequestResend (GlobalId unitId) const { return true; }
		virtual bool XCanTerminateTree (Node const & node) const { return false; }
	};

	class MemberPolicy : public Policy
	{
	public:
		MemberPolicy (Project::Db const & projectDb)
			: _projectDb (projectDb)
		{}

		bool XCanRequestResend (GlobalId unitId) const;
		bool XCanTerminateTree (Node const & node) const;

	private:
		Project::Db const &	_projectDb;
	};
}

#endif
