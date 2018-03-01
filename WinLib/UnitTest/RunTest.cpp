//-------------------------------
//  (c) Reliable Software, 2005-6
//-------------------------------
#include "precompiled.h"
#include <ostream>

#include <Xml/XmlTree.h>
#include <Sys/Active.h>

namespace UnitTest 
{ 
	void XmlTree (std::ostream & out);
	void ActiveObjectTest (std::ostream & out);
	void SmtpLineBreaker (std::ostream & out);
	void LongSequenceTest (std::ostream & out);
	void RegistryTest (std::ostream & out);
	void MultiStringTest (std::ostream & out);
	void DateTest (std::ostream & out);
	void CryptographyTest (std::ostream & out);
	void FullPathSeqTest (std::ostream & out);
}

void RunTest (std::ostream & out)
{
#if !defined (NDEBUG) && !defined (BETA)
	UnitTest::LongSequenceTest (out);
	UnitTest::RegistryTest (out);
	UnitTest::MultiStringTest (out);
	UnitTest::SmtpLineBreaker (out);
	UnitTest::XmlTree (out);
	UnitTest::ActiveObjectTest (out);
	UnitTest::DateTest (out);
	UnitTest::CryptographyTest (out);
	UnitTest::FullPathSeqTest (out);
#endif
}
