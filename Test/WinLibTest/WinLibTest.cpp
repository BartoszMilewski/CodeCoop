// --------------------------
// (c) Reliable Software 2005
// --------------------------
#include "precompiled.h"

#include "WinOut.h"
#include <StringOp.h>
#include <sstream>

namespace UnitTest 
{ 
	extern void Base64Coding (unsigned int startLength, unsigned int finishLength);
	void Base64CodingInPop3AndSmtp (unsigned int startLength, unsigned int finishLength); 
	void XmlTree (std::ostream & out);
}

class Timer 
{
public:
	Timer (WinOut & out) : _out (out), _start (0) {}
	void Start () { _start = clock (); }
	void Measure ()
	{
		clock_t finish = clock ();
		int duration = (finish - _start) / CLOCKS_PER_SEC;
		std::string info = "Test duration: ";
		info += ToString (duration);
		info += " seconds.";
		_out.PutLine (info.c_str ());
	}
private:
	WinOut	& _out;
	clock_t   _start;
};

int RunTest (WinOut & out)
{
	out.PutLine ("Begin Test");
	try
	{
		std::ostringstream buffer; 
		UnitTest::XmlTree (buffer);
		out.PutMultiLine (buffer.str ());
#if 0		
		Timer timer (out);

		out.PutLine ("Start Base64 coding test.");
		timer.Start ();
		UnitTest::Base64Coding (1000000, 1000010);
		out.PutLine ("Finished Base64 coding test.");
		timer.Measure ();

		out.PutLine ("Start Base64 coding in Pop3 and Smtp test.");
		timer.Start ();
		UnitTest::Base64CodingInPop3AndSmtp (1000000, 1000010);
		out.PutLine ("Finished Base64 coding in Pop3 and Smtp test.");
		timer.Measure ();
#endif
	}
	catch (char const * msg)
	{
		TheOutput.Display (msg);
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch (...)
	{
		TheOutput.Display ("Unexpected error", Out::Error);
	}
	out.PutLine ("End Test");
	return 0;
}
