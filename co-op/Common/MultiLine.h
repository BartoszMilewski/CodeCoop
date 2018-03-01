#if !defined (MULTILINE_H)
#define MULTILINE_H
//-------------------------------------
//  (c) Reliable Software, 1998 -- 2003
//-------------------------------------

//
// Long comment as line sequence
//

class MultiLineComment
{
public:
	MultiLineComment ()
	{}
    MultiLineComment (std::string const & comment, unsigned int maxLineLength = 0xffffffff);	// Default no limit
	void Init (std::string const & comment, unsigned int maxLineLength = 0xffffffff);	// Default no limit

	std::string GetFirstLine () const 
	{
		if (_lines.size () !=0)
			return _lines [0];
		return std::string ();
	}

	class Sequencer
	{
	public:
		Sequencer (MultiLineComment const & comment)
			: _cur (comment._lines.begin ()),
			  _end (comment._lines.end ())
		{}

		bool AtEnd () const { return _cur == _end; }
		void Advance () { ++_cur; }

		std::string const & GetLine () const { return *_cur; }

	private:
		std::vector<std::string>::const_iterator	_cur;
		std::vector<std::string>::const_iterator	_end;
	};

	friend class Sequencer;

private:
    std::vector<std::string>	_lines;
};

#endif
