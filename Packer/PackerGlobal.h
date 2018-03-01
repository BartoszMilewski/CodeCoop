#if !defined (PACKERGLOBAL_H)
#define PACKERGLOBAL_H
//-----------------------------------------------------
//  PackerGlobal.h
//  (c) Reliable Software 2001
//-----------------------------------------------------

enum
{
	ByteBits = std::numeric_limits<unsigned char>::digits,
	LongBits = std::numeric_limits<unsigned long>::digits,
	Version = 1,
	VersionBits = 4
};

#endif
