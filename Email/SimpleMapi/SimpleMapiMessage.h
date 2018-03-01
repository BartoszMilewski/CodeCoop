#if !defined (SIMPLEMAPIMESSAGE_H)
#define SIMPLEMAPIMESSAGE_H
// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------

#include "SimpleMapi.h"

class OutgoingMessage;

namespace SimpleMapi
{
	class Message
	{
	public:
		Message (OutgoingMessage const & blueprint, 
			MapiRecipDesc const * recipients, 
			unsigned int recipientCount);

		MapiMessage * ToNative () { return &_msg; }

	private:
		MapiMessage				  _msg;
		std::vector<MapiFileDesc> _attachments;
	};
}

#endif
