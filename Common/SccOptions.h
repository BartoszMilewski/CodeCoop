#if !defined (SCCOPTIONS_H)
#define SCCOPTIONS_H
//------------------------------------
//  (c) Reliable Software, 2000 - 2006
//------------------------------------

#include <bitset>

namespace CodeCoop
{
	// Note: delete from disk is equal to 1 
	// in the current implementation of standard library!
	// Potential problem for clients!
	class SccOptions
	{
	public:
		SccOptions ()
		{}
		explicit SccOptions (void * pvOptions)
			: _flags (static_cast<unsigned long long>(reinterpret_cast<unsigned long>(pvOptions)))
		{}
		explicit SccOptions (unsigned long long value)
			: _flags (value)
		{}
		void SetRecursive ()			{ _flags.set (recursive); }
		void SetAllInProject ()			{ _flags.set (all); }
		void SetKeepCheckedOut ()		{ _flags.set (keepCheckedOut); }
		void SetTypeHeader ()			{ _flags.set (headerFile); }
		void SetTypeSource ()			{ _flags.set (sourceFile); }
		void SetTypeText ()				{ _flags.set (textFile); }
		void SetTypeBinary ()			{ _flags.set (binaryFile); }
		void SetCurrent ()				{ _flags.set (current); }
		void SetDontDeleteFromDisk ()	{ _flags.set (dontDeleteFromDisk); }
		void SetDontCheckIn ()			{ _flags.set (dontCheckIn); }

		bool IsRecursive () const		{ return _flags.test (recursive); }
		bool IsAllInProject () const	{ return _flags.test (all); }
		bool IsKeepCheckedOut () const	{ return _flags.test (keepCheckedOut); }
		bool IsTypeHeader () const		{ return _flags.test (headerFile); }
		bool IsTypeSource () const		{ return _flags.test (sourceFile); }
		bool IsTypeText () const		{ return _flags.test (textFile); }
		bool IsTypeBinary () const		{ return _flags.test (binaryFile); }
		bool IsCurrent () const			{ return _flags.test (current); }
		bool IsDontDeleteFromDisk () const { return _flags.test (dontDeleteFromDisk); }
		bool IsDontCheckIn () const		{ return _flags.test (dontCheckIn); }

		unsigned long GetValue () const { return _flags.to_ulong (); }

	private:
		enum
		{
			dontDeleteFromDisk = 0, // don't change. callers use this flag
			recursive,
			all,
			keepCheckedOut,
			headerFile,
			sourceFile,
			textFile,
			binaryFile,
			current,
			dontCheckIn, // used with SccAdd
			flagCount
		};

	private:
		std::bitset<flagCount>	_flags;		// Command flags
	};
}

#endif
