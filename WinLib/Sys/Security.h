#if !defined (SECURITY_H)
#define SECURITY_H
// Reliable Software (c) 2004
#include <aclapi.h>
class File;

namespace Security
{
	namespace Authority
	{
		// Predefined authorities
		// SID_IDENTIFIER_AUTHORITY is a struct, the initializers are macros
		const SID_IDENTIFIER_AUTHORITY	Null = SECURITY_NULL_SID_AUTHORITY;
		const SID_IDENTIFIER_AUTHORITY	World = SECURITY_WORLD_SID_AUTHORITY;
		const SID_IDENTIFIER_AUTHORITY	Local = SECURITY_LOCAL_SID_AUTHORITY;
		const SID_IDENTIFIER_AUTHORITY	Creator = SECURITY_CREATOR_SID_AUTHORITY;
		const SID_IDENTIFIER_AUTHORITY	NonUnique = SECURITY_NON_UNIQUE_AUTHORITY;
		const SID_IDENTIFIER_AUTHORITY	ResourceManager = SECURITY_RESOURCE_MANAGER_AUTHORITY;
	}

	// Security::Identifier (SID) -- base class
	class Identifier
	{
	public:
		Identifier (SID_IDENTIFIER_AUTHORITY const & authority)
			: _authority (authority), _sid (0)
		{}
		PSID Get () const { return _sid; }
	protected:
		~Identifier ();
		void AddSubAuthority (DWORD subA);
		// Must call Allocate before using
		void Allocate ();
	private:
		SID_IDENTIFIER_AUTHORITY	_authority;
		std::vector<DWORD>			_subAuthority;
		PSID						_sid;
	};

	// Security indentifier of a trustee
	class TrusteeId: public Security::Identifier
	{
	public:
		TrusteeId (SID_IDENTIFIER_AUTHORITY const & authority)
			: Security::Identifier (authority)
		{}
		TRUSTEE_FORM Form () const { return TRUSTEE_IS_SID; }
		TRUSTEE_TYPE Type () const { return TRUSTEE_IS_WELL_KNOWN_GROUP; }
	};

	// Known trustee: Everyone
	class TrusteeEveryone: public TrusteeId
	{
	public:
		TrusteeEveryone ()
			: TrusteeId (Authority::World)
		{
			AddSubAuthority (SECURITY_WORLD_RID);
			Allocate ();
		}
	};

	// Explicit access rights given to a trustee
	// We default here to seting access with all possible rights
	// Only because this is convenient for our purposes
	class ExplicitAccess: public EXPLICIT_ACCESS
	{
	public:
		ExplicitAccess (TrusteeId & trustee, bool isInheritable);
		// add methods to modify the defaults
	};

	class Acl
	{
	public:
		Acl (PACL pAcl = 0) : _pAcl (pAcl) {}
		Acl (File & file);
		ACL * Get () const { return _pAcl; }
	protected:
		ACL * _pAcl;
	};

	// Access Control List (ACL) that uses a vector of explicit accesses
	class AccessControlList: public Acl
	{
	public:
		AccessControlList (std::vector<ExplicitAccess> & explAcc, Acl oldAcl = Acl ());
		~AccessControlList ();
	};

	// Security::Descriptor contains security information
	// associated with a securable object (file, share, registry key, etc...)
	class Descriptor 
	{
	protected:
		Descriptor () : _security (0) {}
	public:
		~Descriptor ();
		PSECURITY_DESCRIPTOR Get () const { return _security; }
		void SetAcl (Acl acl);
	protected:
		PSECURITY_DESCRIPTOR _security;
	};

	// Security descriptor for a new object
	class DescriptorNew: public Descriptor
	{
	public:
		DescriptorNew ();
	};

	// Security descriptor for an existing local share
	class DescriptorLocalShare: public Descriptor
	{
	public:
		// don't include machine name
		DescriptorLocalShare (std::string const & name);
	};
}
#endif

