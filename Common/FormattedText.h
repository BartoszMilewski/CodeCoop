#if !defined (FORMATTEDTEXT_H)
#define FORMATTEDTEXT_H
//---------------------------------------
//  FormattedText.h
//  (c) Reliable Software, 2002
//---------------------------------------

#include "DumpWin.h"

class FormattedText
{
	typedef std::vector<std::pair<std::string, DumpWindow::Style> >::const_iterator Iterator;

public:
	FormattedText () {}

	void AddLine (std::string const & line, DumpWindow::Style style = DumpWindow::styNormal)
	{
		_lines.push_back (std::make_pair (line, style));
	}

	class Sequencer
	{
	public:
		Sequencer (FormattedText const & text)
			: _cur (text._lines.begin ()),
			  _end (text._lines.end ())
		{}

		bool AtEnd () const { return _cur == _end; }
		void Advance () { ++_cur; }

		std::string const & GetLine () const { return _cur->first; }
		DumpWindow::Style GetStyle () const { return _cur->second; }

	private:
		Iterator	_cur;
		Iterator	_end;
	};

	friend class Sequencer;

private:
	std::vector<std::pair<std::string, DumpWindow::Style> >	_lines;
};

#endif
