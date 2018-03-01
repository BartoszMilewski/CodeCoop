//
// Reliable Software (c) 1998
//

#include "precompiled.h"
#include "crypt.h"
#include "stdlib.h"
#include <StringOp.h>
#include <iostream>

int main (int count, char const * args [])
{
#if 0
	extern int char2num (char c);
	extern char num2char (int n);
	for (int i = 0; i < 64; i++)
	{
		char c = num2char (i);
		int x  = char2num (c);
		if (x != i)
			std::cout << "Mismatch " << i << " turned into " << c << " and then into " << x << std::endl;
	}
#endif
	if (count < 2)
	{
		std::cerr << "Usage: license \"licensee name\" seats product\n";
		std::cerr << "where product is P, L, or W\n";
		std::cerr << "Or:    license -m numLicenses startingSerNum seatsEach\n";
		return 1;
	}

	char const * name = args [1];
	if (name [0] == '-' && name [1] == 'm')
	{
		if (count < 5)
		{
			std::cerr << "Usage: license -m numLicenses startingSerNum seatsEach\n";
			return 1;
		}
		int numLicenses = atoi (args [2]);
		int serNum = atoi (args [3]);
		int seats = atoi (args [4]);
		char product = 'P';
		if (count == 6)
		{
			product = args [5][0];
		}
		std::cerr << "Generating " << numLicenses << " licenses, " << seats << " seats each, ";
		std::cerr << "Starting serial number: " << serNum << std::endl;
		for (int i = 0; i < numLicenses; ++i)
		{
			std::string licensee = ToString (serNum + i);
			std::cout << licensee << "\t";
			std::cout << EncodeLicense10 (licensee, seats, product) << std::endl;
		}
	}
	else
	{
		int seats = 1;
		if (count > 2)
			seats = atoi (args [2]);
		int len = strlen (name);
		if (name [0] == '\"')
		{
			name++;
			len--;
		}
		if (name [len - 1] == '\"')
			len--;
		if (len > 90)
		{
			std::cerr << "Name too long\n";
			return 2;
		}
		// Product
		char product = 'P';
		if (count == 4)
		{
			product = args [3][0];
		}
		std::string licensee (name, len);
		std::string key = EncodeLicense10 (licensee, seats, product);
		std::cout << licensee << std::endl;
		std::cout << key << std::endl;
#if 1
		extern bool Decode10 (std::string name, std::string key, int & version, int & seats, char & product);
		int xVersion;
		int xSeats;
		char xProduct;
		if (!Decode10 (licensee, key, xVersion, xSeats, xProduct))
			std::cerr << "Error: Decoding failed!\n";
		if (xVersion != 5)
			std::cerr << "Error: Bad version number encoding\n";
		if (xSeats != seats)
			std::cerr << "Error: Bad seat number encoding\n";
#endif
	}
	return 0;
}
