//
// (c) Reliable Software 1998, 99, 2000, 01
//

#include "precompiled.h"
#include "MapiProperty.h"

#include <mapioid.h>

// This file no longer in the SDK, so define MAPI_PREFIX directly
//#include <initoid.h>
#define MAPI_PREFIX         0x2A,0x86,0x48,0x86,0xf7,0x14,0x03


using namespace Mapi;

// Cannot use macro OID_MIMETAG from 'mapioid.h'
// because they use 16-bit specific Microsoft C Compiler
// keywords not supported on 32-bit platform like '__segname("_CODE")'
unsigned char AttachTagMime::_oid [] = { MAPI_PREFIX, OID_TAG, 0x4 };

void BinaryProperty::Init (unsigned long count, unsigned char const * bytes)
{
	_bin.resize (count);
	memcpy (&_bin [0], bytes, count);
	_prop.Value.bin.cb = _bin.size ();
	_prop.Value.bin.lpb = &_bin [0];
}

AttachTagMime::AttachTagMime ()
{
	_prop.Value.bin.cb = sizeof (_oid);
	_prop.Value.bin.lpb = _oid;
	_prop.ulPropTag = PR_ATTACH_TAG;
};

SPropTagArray * PropertyList::Cnv2TagArray ()
{
	// Convert vector to insane MAPI data structure
	unsigned int tagCount = _tags.size ();
	unsigned int byteSize = CbNewSPropTagArray (tagCount);
	_tagArray.reset (reinterpret_cast<SPropTagArray *>(new unsigned char [byteSize]));
	_tagArray->cValues = tagCount;
	for (unsigned int i = 0; i < tagCount; ++i)
	{
		_tagArray->aulPropTag [i] = _tags [i];
	}
	return _tagArray.get ();
}
