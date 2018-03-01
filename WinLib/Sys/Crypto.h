#if !defined (CRYPTO_H)
#define CRYPTO_H
// -----------------------------
// (c) Reliable Software, 2003-6
// -----------------------------
#include <wincrypt.h>

namespace UnitTest { void CryptographyTest (std::ostream & out); }

namespace Crypt
{
	class String
	{
	public:
		String (unsigned char const * data, unsigned dataSize)
		{
			_blob.cbData = dataSize;
			_blob.pbData = static_cast<unsigned char *> (::LocalAlloc (LMEM_FIXED, dataSize));
			// std::memset (_blob.pbData, '*', dataSize);
			std::copy (data, data + dataSize, _blob.pbData);
			Decrypt ();
		}
		String (std::string const & plainText, std::string const & description)
			: _plainText (plainText.begin (), plainText.end ()) // doesn't copy final null
		{
			_plainText.push_back ('\0');
			_blob.cbData = 0;
			_blob.pbData = 0;
			Encrypt (description);
		}
		~String ()
		{
			::LocalFree (_blob.pbData);
		}
		char const * GetPlainText () const
		{
			return reinterpret_cast<char const *> (&_plainText [0]);
		}
		unsigned char const * GetData (unsigned & size)
		{
			size = _blob.cbData;
			return _blob.pbData;
		}
	private:
		void Encrypt (std::string const & description);
		void Decrypt ();
	private:
		DATA_BLOB _blob;
		std::vector<unsigned char> _plainText; // with final null!
	};

	// Cryptographic context for a default crypto provider
	class Context
	{
	public:
		Context ();
		~Context ();
		HCRYPTPROV ToNative () const { return _provider; }
	private:
		HCRYPTPROV _provider;
	};

	class Hash
	{
	public:
		Hash (Context const & context, std::string const & password);
		~Hash ();
		HCRYPTHASH ToNative () const { return _hash; }
	private:
		HCRYPTHASH _hash;
	};

	class Key
	{
	public:
		~Key ();
		HCRYPTKEY ToNative () const { return _key; }
	protected:
		Key (Crypt::Context const & context, ALG_ID algorithm, DWORD flags);
		Key (Crypt::Context const & context, Crypt::Hash const & hash, ALG_ID algorithm, DWORD flags);
	private:
		HCRYPTKEY _key;
	};

	// RC4 symmetric stream algorithm with 128-bit key
	class KeyRC4: public Key
	{
		static unsigned const KEYLEN = 128;
	public:
		KeyRC4 (Context const & context, Hash const & hash)
			: Key (context, hash, CALG_RC4, KEYLEN << 16) 
		{}
		KeyRC4 (Context const & context)
			: Key (context, CALG_RC4, KEYLEN << 16) 
		{}
	private:
		HCRYPTKEY _key;
	};

	class Streamer
	{
		friend void UnitTest::CryptographyTest (std::ostream & out);
	public:
		Streamer (std::string const & password);
		void SetInput (char const * buf, unsigned bufSize)
		{
			Assert (buf != 0);
			Assert (bufSize > 0);
			_inSize = bufSize;
			_inBuf = buf;
		}
		void Encrypt (std::ostream & out);
		void Decrypt (std::ostream & out);
	private:
		static unsigned const ENCRYPT_BLOCK_SIZE = 8;
	private:
		Crypt::Context	_context;
		Crypt::Hash		_hash;
		Crypt::KeyRC4	_key;

		// input
		char const *	_inBuf;
		unsigned		_inSize;

		// transfer buffer
		unsigned		_inBlockLen;
		std::vector<unsigned char> _buf;
	};
}

#endif
