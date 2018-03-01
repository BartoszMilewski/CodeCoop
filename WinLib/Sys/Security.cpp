// Reliable Software (c) 2004
#include <WinLibBase.h>
#include <File/File.h>
#define UNICODE
#include <Sys/Security.h>

using namespace Security;

Identifier::~Identifier ()
{
	// Advapi32.lib
	::FreeSid (_sid);
}

void Identifier::AddSubAuthority (DWORD subA)
{
	if (_subAuthority.size () == 8)
		throw Win::Exception ("SID: Too many subauthorities", 0, 0);
	_subAuthority.push_back (subA);
}

void Identifier::Allocate ()
{
	BYTE count = static_cast<BYTE> (_subAuthority.size ());
	_subAuthority.resize (8);
	// Advapi32.lib
	if (!::AllocateAndInitializeSid (&_authority,
										count,
										_subAuthority [0],
										_subAuthority [1],
										_subAuthority [2],
										_subAuthority [3],
										_subAuthority [4],
										_subAuthority [5],
										_subAuthority [6],
										_subAuthority [7],
										&_sid))
	{
		throw Win::Exception ("Cannot Initialize a Security Identifier");
	}
}

ExplicitAccess::ExplicitAccess (TrusteeId & trustee, bool isInheritable)
{
	memset (this, 0, sizeof (EXPLICIT_ACCESS));
	grfAccessPermissions = STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL;
	grfAccessMode = SET_ACCESS;
	grfInheritance= isInheritable ? SUB_CONTAINERS_AND_OBJECTS_INHERIT : NO_INHERITANCE;
	Trustee.TrusteeForm = trustee.Form ();
	Trustee.TrusteeType = trustee.Type ();
	Trustee.ptstrName  = (LPWSTR) trustee.Get ();
}

Acl::Acl (File & file)
	: _pAcl (0)
{
	DWORD stat = ::GetSecurityInfo (file.ToNative (), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, 0, 0, &_pAcl, 0, 0);
	if (stat != ERROR_SUCCESS)
		throw Win::Exception ("Cannot get security info", "File/Directory", stat);
}

AccessControlList::AccessControlList (std::vector<ExplicitAccess> & explAcc, Acl oldAcl)
{
	// Advapi32.lib
	DWORD res = ::SetEntriesInAcl (explAcc.size (), &explAcc [0], oldAcl.Get (), &_pAcl);
	if (res != ERROR_SUCCESS)
		throw Win::Exception ("SetEntriesInAcl error");
}

AccessControlList::~AccessControlList ()
{
	::LocalFree (_pAcl);
}

Descriptor::~Descriptor ()
{
	::LocalFree (_security);
}

void Descriptor::SetAcl (Acl acl)
{
	// Sets the discretionary access list
	// Advapi32.lib
	if (!::SetSecurityDescriptorDacl (_security,
									TRUE, // bDaclPresent
									acl.Get (),
									FALSE)) // not a default acl
	{
		throw Win::Exception ("Could not set access control list");
	}
}

DescriptorNew::DescriptorNew ()
{
	_security = (PSECURITY_DESCRIPTOR) ::LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
	// Advapi32.lib
	if (!::InitializeSecurityDescriptor (_security, SECURITY_DESCRIPTOR_REVISION))
		throw Win::Exception ("Could not initialize security descriptor");
}

DescriptorLocalShare::DescriptorLocalShare (std::string const & name)
{
	std::wstring wName;
	wName.assign (name.begin (), name.end ());
	// Advapi32.lib
	if (!::GetNamedSecurityInfo(&wName [0],
								SE_LMSHARE, // type
								0, // security information: just the descriptor
								0, // PSID* ppsidOwner,
								0, // PSID* ppsidGroup,
								0, // PACL* ppDacl,
								0, // PACL* ppSacl,
								&_security))
	{
		throw Win::Exception ("Cannot get security descriptor for local share", name.c_str ());
	}
}
