// -----------------------------
// (c) Reliable Software, 2005-6
// -----------------------------

#include <WinLibBase.h>
#include "EmailAddress.h"

bool Email::IsValidAddress (std::string const & addr)
{
	// see RFC #2821 and RFC#2822 for strict e-mail address specification
	return !addr.empty () &&
		   (addr [0] != '@') &&
		   (addr.find ('@', 1) != std::string::npos) && 
		   (addr [addr.size () - 1] != '@') &&
		   (addr.find (' ') == std::string::npos);
}
