//----------------------------------
//  (c) Reliable Software, 2005
//----------------------------------
#include <WinLibBase.h>
#include "LongSequence.h"
#include <ostream>

namespace UnitTest
{
	class Item
	{
	public:
		Item (int x) : _x (x)
		{}
		int Get () const { return _x; }
	private:
		int _x;
	};

	class NumberGenerator
	{
	public:
		NumberGenerator (unsigned seed)
		{
			std::srand (seed);
		}
		int Get ()
		{
			return std::rand ();
		}
	};

	void TestGenerator (std::ostream & out)
	{
		NumberGenerator gen (123);
		std::vector<int> v;
		for (int i = 0; i < 200; ++i)
			v.push_back (gen.Get ());
		NumberGenerator gen2 (123);
		for (std::vector<int>::const_iterator it = v.begin (); it != v.end (); ++it)
		{
			Assert (*it == gen2.Get ());
			// out << *it << ", ";
		}
		out << "Number generator test: passed" << std::endl;
	}

	typedef LongSequence<unsigned char> ByteSeq;
	typedef LongSequence<Item> ItemSeq;

	void SmallTest (ByteSeq & s, unsigned char a [2])
	{
		Assert (s.begin () == s.end ());
		s.push_back (a [0]);
		{
			ByteSeq::iterator it = s.begin ();
			Assert (it != s.end ());
			Assert (*it == a [0]);
			++it;
			Assert (it == s.end ());
		}
		// s.Flush ();
		s.push_back (a [1]);
		{
			ByteSeq::iterator it = s.begin ();
			Assert (it != s.end ());
			Assert (*it == a [0]);
			++it;
			Assert (it != s.end ());
			Assert (*it == a [1]);
			++it;
			Assert (it == s.end ());
		}
	}

	void SmallTestResult (ByteSeq const & s, unsigned char a [2])
	{
		ByteSeq::iterator it = s.begin ();
		Assert (it != s.end ());
		Assert (*it == a [0]);
		++it;
		Assert (it != s.end ());
		Assert (*it == a [1]);
		++it;
		Assert (it == s.end ());
	}

	void RandomWrite (LongSequence<Item> & seq, unsigned countLow, unsigned countHigh)
	{
		NumberGenerator gen (0);
		LargeInteger count (countLow, countHigh);
		for (LargeInteger i = 0; i < count; ++i)
			seq.push_back (gen.Get ());
	}

	void RandomTest (LongSequence<Item> & seq, unsigned countLow, unsigned countHigh)
	{
		NumberGenerator gen (0);
		LongSequence<Item>::iterator it = seq.begin (), end = seq.end ();
		LargeInteger i = 0;
		LargeInteger count (countLow, countHigh);
		while (it != end)
		{
			Assert (it->Get () == gen.Get ());
			++it;
			++i;
		}
		Assert (i == count);
	}

	void RandomTestFile (std::ostream & out, std::string const & path, unsigned countLow, unsigned countHigh)
	{
		out << "Testing with large file" << std::endl;
		{
			out << "Writing [" << countLow << ", " << countHigh << "] records to file." << std::endl;
			LongSequence<Item> seq (path);
			RandomWrite (seq, countLow, countHigh);
			out << "Reading back from Long Sequence" << std::endl;
			RandomTest (seq, countLow, countHigh);
		}
		{
			out << "Reading [" << countLow << ", " << countHigh << "] records from file." << std::endl;
			LongSequence<Item> seq (path, File::ReadOnlyMode ());
			RandomTest (seq, countLow, countHigh);
			out << "Passed!" << std::endl;
		}
	}

	void RandomTestSwapFile (std::ostream & out, unsigned countLow, unsigned countHigh)
	{
		out << "Testing with large sequence" << std::endl;
		{
			out << "Pushing [" << countLow << ", " << countHigh << "] records." << std::endl;
			File::Size size (countLow, countHigh);
			size *= sizeof (Item);
			LongSequence<Item> seq (size);
			RandomWrite (seq, countLow, countHigh);
			out << "Iterating records" << std::endl;
			RandomTest (seq, countLow, countHigh);
		}
	}

	void TestWithFile (std::string const & path, std::ostream & out)
	{
		out << "=== Long Sequence with file ===" << std::endl;
		File::Delete (path);
		{
			// Write empty sequence
			ByteSeq s (path);
		}
		{
			// Read empty sequence
			ByteSeq s (path, File::ReadOnlyMode ());
			Assert (s.begin () == s.end ());
			out << "Empty file sequence passed." << std::endl;
		}
		unsigned char a [2] = { 'a', 'b' };
		{
			ByteSeq s (path);
			SmallTest (s, a);
		}
		{
			ByteSeq s (path, File::ReadOnlyMode ());
			SmallTestResult (s, a);
			out << "Two byte file sequence passed." << std::endl;
		}
		File::Delete (path);
		RandomTestFile (out, path, 0x100000, 0);
		File::Delete (path);
		RandomTestFile (out, path, 0x100000 - 1, 0);
		File::Delete (path);
		RandomTestFile (out, path, 0x100000 + 1, 0);
		File::Delete (path);
		//RandomTestFile (out, path, 400000000, 0);
	}

	void TestWithSwapFile (std::ostream & out)
	{
		out << "=== Long Sequence using swap file ===" << std::endl;
		{
			ByteSeq s (File::Size (1, 0));
			Assert (s.begin () == s.end ());
			out << "Empty sequence passed." << std::endl;
		}
		unsigned char a [2] = { 'a', 'b' };
		{
			ByteSeq s (File::Size (100, 0));
			SmallTest (s, a);
			out << "Two byte sequence passed." << std::endl;
		}
		RandomTestSwapFile (out, 0x100000, 0);
		RandomTestSwapFile (out, 0x100000 - 1, 0);
		RandomTestSwapFile (out, 0x100000 + 1, 0);
	}

	void LongSequenceTest (std::ostream & out)
	{
		out << "LongSequence test" << std::endl;
		SYSTEM_INFO info;
		::GetSystemInfo (&info);
		unsigned granularity = info.dwAllocationGranularity;
		Assert (PageSize % granularity == 0);

		TestGenerator (out);
		TestWithSwapFile (out);
		TestWithFile ("foo.txt", out);
		out << "=== Long Sequence test passed!" << std::endl << std::endl;
	}
}