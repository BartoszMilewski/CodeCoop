#if !defined (COMMANDPARAMS_H)
#define COMMANDPARAMS_H
//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include <Xml/XmlTree.h>

namespace CmdLine
{
    // Command-line switch tokens
    enum SwitchToken
    {
		ProjectFile,
		BeforeFile,
		AfterFile,
		ReferenceFile,

		XmlHandle,
		Mode,
		Title,
		Comment,
		EditLog,
		ShowWhat,
		Status,
		ShortInfo,
		LastSwitch
    };

    class CommandParams
    {
    public:
      
        CommandParams (char const * args = 0)
            : _fileCount (0)
			, _xmlArgs (new XML::Tree)
        {
            if (args)
                Init (args);
        }
        void Init (char const * params);
        int  GetCount () const { return _strings.size (); }
		bool IsSet (SwitchToken tok, char const * value) const;
        char const * GetString (SwitchToken tok) const;
		void Clear ();
		// Warning: call once
		std::unique_ptr<XML::Tree> ReleaseTree () { return std::move(_xmlArgs);} 

	private:
		void AddFile (std::string const & path, SwitchToken token);
		typedef std::map<SwitchToken, std::string> TokenStringMap;
	private:
		std::unique_ptr<XML::Tree> _xmlArgs;
        int				_fileCount;
		TokenStringMap	_strings;
    };

    class SwitchMap
    {
    public:
        static SwitchToken CharToToken (int c);
        static int TokenToChar (SwitchToken tok);
		static int TokenLen (SwitchToken tok) { return 4; } // Switch token length in chars

    public:
        static SwitchMap   _switchChars [LastSwitch];

        SwitchToken _token;
        int         _char;
    };

    class StringParser
    {
    public:
        StringParser (char const * string)
            : _string (string), _curOffset (0), _beginning (0)
        {}
        bool AtEnd () const { return _string [_curOffset] == '\0'; }
        int  GetChar () const { return _string [_curOffset]; }
        void SkipChar () { _curOffset++; }
		void EatWord ();
        void EatWhite ();
        bool FindQuote ();
        void FindWhite ();
        void MarkBeginning () { _beginning = _curOffset; }
        int  GetCurLen () const { return _curOffset - _beginning; }
        char const * GetBeginning () const { return &_string [_beginning]; }
		bool GetHexNumber (unsigned int & number);
    private:
        char const *_string;
        int         _curOffset;
        int         _beginning;
    };

}; // CmdLine namespace

#endif
