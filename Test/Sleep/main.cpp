#include <stdlib.h>
#include <windows.h>

int main (int count, char * arg [])
{
	int interval = 10 * 1000; // 10 sec
	if (count == 2)
	{
		// user specified interval
		interval = ::atoi (arg [1]) * 1000;
	}
	::Sleep (interval);
	return 0;
}
