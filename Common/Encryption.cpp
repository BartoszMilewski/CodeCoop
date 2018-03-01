// ---------------------------
// (c) Reliable Software, 2006
// ---------------------------
#include "precompiled.h"
#include "Encryption.h"

Encryption::KeyMan::KeyMan (Catalog & cat, std::string const & projName)
: _cat (cat)
{
	// Revisit: implement
	// iterate over key list, 
	// find key for the given project, 
	// check for and remember a common key
}

void Encryption::KeyMan::SetKey (std::string const & key)
{
	// Revisit: implement
}
