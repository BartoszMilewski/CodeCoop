#include "precompiled.h"
#include <XML/Scanner.h>
#include <XML/XmlParser.h>
#include <XML/XmlTree.h>
#include <Ex/WinEx.h>
#include <iostream>
#include <fstream>
#include <cassert>

int main (int argc, char ** argv)
{
	if (argc < 2)
	{
		std::cerr << "Usage: extract filename.html\n";
		return 1;
	}
	std::ifstream in (argv [1]);
	if (!in.good ())
	{
		std::cerr << "Could not open file: " << argv [1] << std::endl;
		return 2;
	}
	try
	{
		XML::Scanner scanner (in);
		XML::Tree tree;
		XML::TreeMaker treeMaker (tree);
		XML::Parser parser (scanner, treeMaker);
		parser.Parse ();

		std::string dummy;
		std::cin >> dummy;
	} 
	catch (Win::InternalException ie)
	{
		std::cout << ie.GetMessage () << std::endl;
	}
	catch (...)
	{
		std::cerr << "Unknown error..." << std::endl;
	}
	return 0;
}