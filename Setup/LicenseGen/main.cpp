// (c) Reliable Software, 2001

#include <WinLibBase.h>
#include <File/MemFile.h>
#include <Ex/WinEx.h>
#include <iostream>

using namespace std;

int main (int argc, char ** argv)
{
	if (argc < 3)
	{
		cout << "Usage: LicenseGen \"user\" \"license\"\n";
	}
	else
	{
		cout << argv [1] << endl << argv [2] << endl;

		string userLicense (argv [1]);
		userLicense += "\n";
		userLicense += argv [2];

		unsigned len = userLicense.size ();
		char key [] = "Our product is best!";
		unsigned keyLen = strlen (key);
		try
		{
			MemFileNew outFile ("co-op-setup.bin", File::Size (len, 0));
			char * buf = outFile.GetBuf ();
			for (unsigned i = 0; i < len; ++i)
			{
				buf [i] = userLicense [i] + 13 + key [i % keyLen];
			}
		}
		catch (Win::Exception e)
		{
			cerr << e.GetMessage () << endl;
		}
		catch (...)
		{
			cerr << "Unexpected exception" << endl;
		}
	}
	return 0;
}