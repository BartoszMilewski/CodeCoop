// ----------------------------------
// (c) Reliable Software, 2003 - 2007
// ----------------------------------
#include <WinLibBase.h>
#include "Crypto.h"
#include "StringOp.h"
#include <Sys/SysVer.h>
#include <File/MemFile.h> // required by unit test
#include <File/Path.h>

namespace Crypt
{
	void CheckVersion ()
	{
		SystemVersion ver;
		if (!ver.IsOK ())
			throw Win::Exception ("Get system version failed");
		if (!ver.IsWinNT ())
			throw Win::Exception ("Password encryption not supported on this platform");
	}
}

void Crypt::String::Encrypt (std::string const & description)
{
	Assert (description.size () != 0);
	CheckVersion ();
	DATA_BLOB blobIn;
	blobIn.cbData = _plainText.size (); // if string, includes final null
	blobIn.pbData = &_plainText [0];
	::CryptProtectData (&blobIn, 
					ToWString (description).c_str (), // description
					0, // entropy
					0, // reserved
					0, // prompt
					0, // flags (only currently logged user can decrypt)
					&_blob);
}

void Crypt::String::Decrypt ()
{
	CheckVersion ();
	DATA_BLOB blobOut;
	Assert (_blob.pbData != 0);
	if (::CryptUnprotectData (&_blob, 
					0, // description
					0, // entropy
					0, // reserved
					0, // prompt
					0, // flags (only currently logged user can decrypt)
					&blobOut) == FALSE)
	{
		throw Win::Exception ("Decryption of local data failed");
	}
	Assume (blobOut.cbData > 0, "If string, includes final null"); // if string, includes final null
	_plainText.resize (blobOut.cbData);
	std::copy (blobOut.pbData, blobOut.pbData + blobOut.cbData, &_plainText [0]);
	::LocalFree (blobOut.pbData);
}

//-------------
// Cryptography
//-------------

Crypt::Context::Context ()
	: _provider (0)
{
	if(!::CryptAcquireContext(
					&_provider, 
					NULL, 
					MS_ENHANCED_PROV, 
					PROV_RSA_FULL, 
					0))
	{
		if (GetLastError() == NTE_BAD_KEYSET)
		{
			if(!::CryptAcquireContext(
							&_provider, 
							NULL, 
							MS_ENHANCED_PROV, 
							PROV_RSA_FULL, 
							CRYPT_NEWKEYSET))
			{
				throw Win::Exception ("Cannot create cryptographic key set");
			}
		}
		else
			throw Win::Exception ("Cannot access cryptographic provider");
	}
}

Crypt::Context::~Context ()
{
	if(_provider != 0)
		::CryptReleaseContext(_provider, 0);
}

Crypt::Hash::Hash (Crypt::Context const & context, std::string const & str)
: _hash (0)
{
	if(!::CryptCreateHash(
		context.ToNative (), 
		CALG_MD5, 
		0, 
		0, 
		&_hash))
	{
		throw Win::Exception ("Cannot create cryptographic hash");
	}
	if(!::CryptHashData(
		_hash, 
		(BYTE *)str.c_str (), 
		str.size (), 
		0))
	{
		throw Win::Exception ("Cannot hash string");
	}
}

Crypt::Hash::~Hash ()
{
	if(_hash != 0) 
		::CryptDestroyHash(_hash);
}

Crypt::Key::Key (Crypt::Context const & context, ALG_ID algorithm, DWORD flags)
	:_key (0)
{
	if (!::CryptGenKey (context.ToNative (),
		algorithm,
		flags,
		&_key))
	{
		throw Win::Exception ("Can't generate cryptographic key");
	}

}

Crypt::Key::Key (Crypt::Context const & context, Crypt::Hash const & hash, ALG_ID algorithm, DWORD flags)
:_key (0)
{
	if(!::CryptDeriveKey(
		context.ToNative (), 
		algorithm, 
		hash.ToNative (), 
		flags, 
		&_key))
	{
		throw Win::Exception ("Cannot derive cryptographic key");
	}
}

Crypt::Key::~Key ()
{
	if(_key != 0)
		::CryptDestroyKey(_key);
}

Crypt::Streamer::Streamer (std::string const & password)
: _context (), 
  _hash (_context, password), 
  _key (_context, _hash),
  _inBlockLen (1000 - 1000 % ENCRYPT_BLOCK_SIZE) // multiple of block size
{
	// Determine the output size. 
	// Stream cyphers don't change the length
	// If a block cipher is used, output must have room for an extra block. 

	unsigned bufLen = _inBlockLen;
	if (ENCRYPT_BLOCK_SIZE > 1) 
		bufLen += ENCRYPT_BLOCK_SIZE;
	_buf.resize (bufLen);
}

void Crypt::Streamer::Encrypt (std::ostream & out)
{
	bool isLast;
	do 
	{
		isLast = (_inSize <= _inBlockLen);
		unsigned len = isLast? _inSize: _inBlockLen;
		std::copy (_inBuf, _inBuf + len, _buf.begin ());
		_inBuf += len;
		_inSize -= len;
		DWORD outCount = len;
		if(!::CryptEncrypt(
			_key.ToNative (), 
			0, 
			isLast, 
			0, 
			&_buf [0], 
			&outCount, 
			_buf.size ()))
		{
			throw Win::Exception ("Encryption failed");
		}
		std::copy (&_buf [0], &_buf [outCount], std::ostream_iterator<unsigned char> (out));
	} while (!isLast);
}

void Crypt::Streamer::Decrypt (std::ostream & out)
{
	bool isLast;
	do 
	{
		isLast = (_inSize <= _inBlockLen);
		unsigned len = isLast? _inSize: _inBlockLen;
		std::copy (_inBuf, _inBuf + len, _buf.begin ());
		_inBuf += len;
		_inSize -= len;
		DWORD outCount = len;
		if(!::CryptDecrypt(
			_key.ToNative (), 
			0, 
			isLast, 
			0, 
			&_buf [0], 
			&outCount))
		{
			throw Win::Exception ("Encryption failed");
		}
		std::copy (&_buf [0], &_buf [outCount], std::ostream_iterator<unsigned char> (out));
	} while (!isLast);
}

namespace UnitTest
{
	void RunTest (std::string const & src, std::string const & password, std::ostream & out)
	{
		std::string cypher;
		{
			Crypt::Streamer streamer (password);
			streamer.SetInput (&src [0], src.size ());
			std::ostringstream out;
			streamer.Encrypt (out);
			cypher = out.str ();
		}
		std::string decoded;
		{
			Crypt::Streamer streamer (password);
			streamer.SetInput (&cypher [0], cypher.size ());
			std::ostringstream out;
			streamer.Decrypt (out);
			decoded = out.str ();
		}
		Assert (decoded.size () == src.size ());
		Assert (decoded == src);
	}

	void CryptographyTest (std::ostream & out)
	{
		out << std::endl << "Test of cryptography." << std::endl;

		std::string password ("secret");
		// empty strings not allowed
		{
		}
		// one character string
		{
			std::string test ("x");
			RunTest (test, password, out);
		}
		// internal buffer boundaries
		{
			Crypt::Streamer auxStreamer (password);
			unsigned int internalBufSize = auxStreamer._buf.size ();
			for (unsigned int size = internalBufSize - 1; size <= internalBufSize + 1; ++size)
			{
				std::string test (size, 'x');
				RunTest (test, password, out);
			}	
			unsigned int internalBufSizeX10 = 10 * internalBufSize;
			for (unsigned int size = internalBufSizeX10 - 1; 
				 size <= internalBufSizeX10 + 1; 
				 ++size)
			{
				std::string test (size, 'x');
				RunTest (test, password, out);
			}	
		}
		// large file
		{
			SystemFilesPath systemPath;
			systemPath.DirDown ("system32");
			MemFileReadOnly in (systemPath.GetFilePath ("shell32.dll"));
			char const * buf = in.GetBuf ();
			std::string contents;
			std::copy (buf, buf + in.GetBufSize (), std::back_inserter (contents));
			RunTest (contents, password, out);
		}

		out << "Passed." << std::endl;
	}
}
