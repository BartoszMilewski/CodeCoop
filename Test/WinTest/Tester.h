#if !defined (TESTER_H)
#define TESTER_H
// (c) Reliable Software 2003
#include <memory>
class WinOut;

// Tester interface. The testing starts immediately upon construction
class Tester
{
public:
	virtual ~Tester () {}
};

// Factory function creates a specific Tester
extern std::auto_ptr<Tester> StartTest (WinOut & output);

#endif
