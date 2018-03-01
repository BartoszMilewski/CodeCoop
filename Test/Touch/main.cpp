#include "precompiled.h"
#include <File/File.h>
#include <iostream>

int main (int count, char * arg [])
{
	if (count != 2)
	{
		std::cout << "Touch a file to make compiler rebuild it.\n\n";
		std::cout << "touch <filename>\n";
		return 0;
	}

	if (File::TouchNoEx (arg [1]))
		std::cout << "		Touched.\n";
	else
		std::cout << "		Not touched.\n";

	return 0;
}
