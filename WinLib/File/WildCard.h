class Matcher
{
public:
	Matcher () : _isOk (true) {}
	virtual bool IsMatch (char const * name) const = 0;
	bool IsOk () const { return _isOk; }
protected:
	bool	_isOk;
};

class FileMatcher: public Matcher
{
public:
	FileMatcher (char const * pattern, int len);
	FileMatcher (char const * pattern);
	bool IsMatch (char const * name) const;
private:
	bool Parse (char const * begin, char const * end);
	bool MatchPrefix1 (char const* & begin, char const * end) const;
	bool MatchStar1 (char const* & begin, char const * end) const;
	bool MatchDot (char const* & begin, char const * end) const;
	bool MatchPrefix2 (char const* & begin, char const * end) const;
	bool MatchStar2 (char const* & begin, char const * end) const;

	std::string	_prefix;
	bool	_star1;
	bool	_dot;
	std::string	_ext;
	bool	_star2;
};

class FileMultiMatcher: public Matcher
{
public:
	FileMultiMatcher (char const * multiPattern);
	bool IsMatch (char const * name) const;
private:
	static	char const * const _separator;

	std::vector<FileMatcher> _matchers;
};

