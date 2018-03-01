//------------------------------
// (c) Reliable Software 2005
//------------------------------
#include "precompiled.h"
#include "EmailRegistry.h"
#include "RegKeys.h"

bool Registry::IsFullMapiSupported ()
{
	Registry::DefaultEmailClient client;
	return client.IsFullMapiSupported ();
}

bool Registry::CanUseFullMapi ()
{
	return IsFullMapiAvailable () && IsFullMapiSupported ();
}

bool Registry::IsSimpleMapiAvailable ()
{
	// Check if Simple MAPI is available on this machine
	Registry::MAPICheck mapiKey;
	if (!mapiKey.Exists ())
		return false;
	Registry::MAPI mapiSystem;
	return mapiSystem.IsSimpleMapiAvailable ();
}

bool Registry::IsFullMapiAvailable ()
{
	// Check if full MAPI is available on this machine
	Registry::MAPICheck mapiKey;
	if (!mapiKey.Exists ())
		return false;
	Registry::MAPI mapiSystem;
	return mapiSystem.IsFullMapiAvailable ();
}

