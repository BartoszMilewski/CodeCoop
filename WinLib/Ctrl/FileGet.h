#if !defined FILEGET_H
#define FILEGET_H
//----------------------------------
//  (c) Reliable Software, 1996-2003
//----------------------------------

class FileGetter: public OPENFILENAME
{
public:
    FileGetter ();
    ~FileGetter ()
    {
        delete []_buf;
    }

    void SetFileName (char const * file)
    {
        if (file)
            strcpy (_buf, file);
        else
            _buf [0] = '\0';
    }

    void SetFilter (char const * filter, char const * extension = 0)
    {
        lpstrFilter = filter;
        lpstrDefExt = extension;
    }

    void SetInitDir (char const * initDir)
    {
        lpstrInitialDir = initDir; 
    }

    char const * GetInitDir () const { return lpstrInitialDir; }
    char const * GetPath () const { return _buf; }
    char const * GetFileName () const
    {
        return &_buf [nFileOffset];
    }

    int GetDirLen () const // without the trailing backslash
    {
        Assert (nFileOffset >= 2);
        if (_buf [nFileOffset - 1] == '\\')
        {
            // cut off the trailing backslash
            return nFileOffset - 1;
        }
        else
        {
            return nFileOffset;
        }
    }

    bool GetExistingFile (Win::Dow::Handle hwnd, char const * title = 0);

    bool GetNewFile (Win::Dow::Handle hwnd, char const * title = 0);

    bool GetMultipleFiles (Win::Dow::Handle hwnd, char const * title = 0);

private:

    char  * _buf;
    int     _bufLen;
};

class PathIter
{
public:
    PathIter (FileGetter const & getter);
    bool AtEnd () const { return 0 == _buf [_iName]; }
    void Advance ();
    char const * GetPath (char * bufPath, int size);
private:
    char const *    _buf;
    int             _iPath;
    int             _iName;
};

#endif