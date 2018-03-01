#if !defined (WINSTRING_H)
#define WINSTRING_H
//--------------------------------
// (c) Reliable Software 1997-2005
//--------------------------------
class ResString
{
public:
    ResString (Win::Dow::Handle mainWnd, int resId);
    ResString (Win::Instance hInst, int resId);
	ResString () {}
    operator char const * () { return _buf.c_str (); }

private:
	void Load (Win::Instance hInst, int resId);

private:
	std::string _buf;
};

#endif
