//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

#include "precompiled.h"
#include "EmailConfigData.h"
#include "Validators.h"
#include "OutputSink.h"

#include "StringOp.h"

bool EmailConfigData::Validate () const
{
	if (_isAutoReceive)
	{
		if (_receivePeriodInMin < Email::MinAutoReceivePeriodInMin ||
			_receivePeriodInMin > Email::MaxAutoReceivePeriodInMin)
		{
			std::string msg ("Please, specify a valid automatic receive period\n(from ");
			msg += ToString (Email::MinAutoReceivePeriodInMin);
			msg += " to ";
			msg += ToString (Email::MaxAutoReceivePeriodInMin);
			msg += " minutes).";
			TheOutput.Display (msg.c_str ());
			return false;
		}
	}

	if (!ChunkSizeValidator (_maxSize).IsInValidRange ())
	{
		std::string info ("Please, specify a valid maximum size of script attachments\n(from ");
		info += ChunkSizeValidator::GetMinChunkSizeDisplayString ();
		info += " to ";
		info += ChunkSizeValidator::GetMaxChunkSizeDisplayString ();
		info += ").";
		TheOutput.Display (info.c_str ());
		return false;
	}

	return true;
}
