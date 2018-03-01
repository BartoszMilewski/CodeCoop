#include "precompiled.h"
#include <XML/XmlTree.h>
#include <Sys/SharedMem.h>
#include <Sys/Process.h>
#include <File/Path.h>

#include <iostream>
#include <fstream>
#include <sstream>

class RandomUniqueName
{
public:
	RandomUniqueName ();
	RandomUniqueName (unsigned long value);
	RandomUniqueName (std::string const & str);

	unsigned long GetValue () const { return _value; }
	std::string const & GetString () const { return _valueStr; }

private:
	void FormatString ();

private:
	unsigned long	_value;
	std::string		_valueStr;
};

// To use as a stream, create 
// std::ostream out (&xmlBuf);
class XmlBuf: public std::streambuf
{
public:
	XmlBuf ();
	XmlBuf (unsigned int handle);
	unsigned int GetHandle () const { return _sharedMem.GetHandle (); }
	char * GetBuf () { return _data; }
	unsigned GetBufSize () const { return DefaultBufSize; }
	enum
	{
		DefaultBufSize = 4096	// 4kb
	};
	// streambuf
	int sync ();
	int overflow (int ch = EOF);
	int underflow ();

private:
	SharedMem			_sharedMem;
	char *				_data;		// Primary memory buffer
	RandomUniqueName	_name;		// Exchange name
};

char const sourceFile [] = "source.txt";
char const targetFile [] = "target.txt";
char const referenceFile [] = "reference.txt";
char const resultFile [] = "target.txt"; // same as target

bool CallMerger (XML::Tree const & xmlArgs);
void CreateFiles ();
bool CheckResult ();

int main ()
{
	try
	{
		CreateFiles ();
		CurrentFolder curPath;

		XML::Tree xmlArgs;
		XML::Node * root = xmlArgs.SetRoot ("merge");
		root->AddAttribute ("auto", "true");
		XML::Node * child = 0;
		// Current file
		child = root->AddEmptyChild ("file");
		child->AddAttribute ("role", "result");
		child->AddAttribute ("path", curPath.GetFilePath (resultFile));
		child->AddAttribute ("edit", "yes");
		// Target file
		child = root->AddEmptyChild ("file");
		child->AddAttribute ("role", "target");
		child->AddAttribute ("path", curPath.GetFilePath (targetFile));
		child->AddAttribute ("display-path", "Target");
		child->AddAttribute ("edit", "no");
		// Source file
		child = root->AddEmptyChild ("file");
		child->AddAttribute ("role", "source");
		child->AddAttribute ("path", curPath.GetFilePath (sourceFile));
		child->AddAttribute ("display-path", "Source");
		child->AddAttribute ("edit", "no");
		// Reference file
		child = root->AddEmptyChild ("file");
		child->AddAttribute ("role", "reference");
		child->AddAttribute ("path", curPath.GetFilePath (referenceFile));
		child->AddAttribute ("display-path", "Reference");
		child->AddAttribute ("edit", "no");

		if (!CallMerger (xmlArgs))
		{
			std::cerr << "Merger call failed.\n"
				"Make sure BCMerge.exe is on your path or in current directory." << std::endl;
			return -1;
		}

		if (!CheckResult ())
		{
			std::cerr << "Incorrect merge result" << std::endl;
			return -1;
		}
		return 0;
	}
	catch (Win::Exception e)
	{
		std::cerr << e.GetMessage () << std::endl;
		return -1;
	}
	return 0;
}

void CreateFiles ()
{
	std::ofstream ref (referenceFile);
	ref << "line one\n"
		"line two\n"
		"line three\n"
		"line four\n";
	std::ofstream src (sourceFile);
	src << "line one source\n"
		"line two\n"
		"line three\n"
		"line four\n";
	std::ofstream trg (targetFile);
	trg << "line one\n"
		"line two\n"
		"line three\n"
		"line four target\n";
}

bool CheckResult ()
{
	std::ifstream in (resultFile);
	std::string line;
	std::getline (in, line);
	if (line != "line one source")
		return false;
	std::getline (in, line);
	if (line != "line two")
		return false;
	std::getline (in, line);
	if (line != "line three")
		return false;
	std::getline (in, line);
	if (line != "line four target")
		return false;
	return true;
}

bool CallMerger (XML::Tree const & xmlArgs)
{

	XmlBuf buf;
	std::ostream out (&buf); // use buf as streambuf
	xmlArgs.Write (out);
	if (out.fail ())
		throw Win::InternalException ("Differ argument string too long.");

	std::string toolPath = "BCMerge.exe";
	std::ostringstream cmdLine;
	cmdLine << " /xmlspec 0x" << std::hex << buf.GetHandle ();
	Win::ChildProcess process (cmdLine.str ().c_str (), true);	// Inherit parent's handles
	process.SetAppName (toolPath);
	process.ShowMinimizedNotActive ();
	process.Create ();
	process.WaitForDeath (10000);	// Wait 10 seconds
	if (process.IsAlive ())
	{
		std::cerr << "Merger hasn't finished in 10 seconds" << std::endl;
		return false;
	}
	return process.GetExitCode () == 0;
}

// Create new shared buffer
XmlBuf::XmlBuf ()
{
	_sharedMem.Create (DefaultBufSize, _name.GetString ());
	_data = reinterpret_cast<char *> (_sharedMem.GetRawBuf ());
	setp (_data, _data + DefaultBufSize);
	setg (_data, _data, _data + DefaultBufSize);
}

// Opens existing shared buffer
XmlBuf::XmlBuf (unsigned int handle)
{
	_sharedMem.Map (handle);
	_data = reinterpret_cast<char *> (_sharedMem.GetRawBuf ());
	setp (_data, _data + DefaultBufSize);
	setg (_data, _data, _data + DefaultBufSize);
}

// streambuf interface
int XmlBuf::sync ()
{
	char * pch = pptr();
	Assert (pch != 0);
	if (pch - _data >= DefaultBufSize)
		return -1;
	return 0;
}

int XmlBuf::overflow (int ch)
{
	return EOF;
}

int XmlBuf::underflow ()
{
	return  gptr() == egptr() ?
			traits_type::eof() :
			traits_type::to_int_type(*gptr());
}

RandomUniqueName::RandomUniqueName ()
{
	unsigned long v1 = rand ();
	unsigned long v2 = rand ();
	_value = (v1 << 16) | v2;
	FormatString ();
}

RandomUniqueName::RandomUniqueName (unsigned long value)
	: _value (value)
{
	FormatString ();
}

RandomUniqueName::RandomUniqueName (std::string const & str)
	: _valueStr (str)
{
	std::istringstream in (str);
	in >> std::hex >> _value;
}

void RandomUniqueName::FormatString ()
{
    std::ostringstream buffer;
	buffer << "0x" << std::hex << _value;
	_valueStr = buffer.str ();
}

