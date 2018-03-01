#if !defined (HTMLTAG_H)
#define HTMLTAG_H
//------------------------------
// (c) Reliable Software 2000-05
//------------------------------

// Handle HTML tags block structure

#include <ostream>

namespace Html
{
	class Tag
	{
	public:
		Tag (std::ostream & stream, std::string const & tag)
			: _stream (stream),
			  _tag (tag)
		{
			_stream << "<" << _tag.c_str () << ">\r\n";
		}
		~Tag ()
		{
			_stream << "</" << _tag.c_str () << ">\r\n";
		}

		std::ostream & GetStream () const { return _stream; }

	private:
		std::ostream &	_stream;
		std::string		_tag;
	};

	class Document : public Html::Tag
	{
	public:
		Document (std::ostream & stream)
			: Html::Tag (stream, "html")
		{}
	};

	class Body : public Html::Tag
	{
	public:
		Body (Document const & doc)
			: Html::Tag (doc.GetStream (), "body")
		{}
	};

	class Head : public Html::Tag
	{
	public:
		Head (Document const & doc)
			: Html::Tag (doc.GetStream (), "head")
		{}
	};

	class Title : public Html::Tag
	{
	public:
		Title (Head const & head)
			: Html::Tag (head.GetStream (), "title")
		{}
	};

	class Heading1 : public Html::Tag
	{
	public:
		Heading1 (Body const & body)
			: Html::Tag (body.GetStream (), "h1")
		{}
	};

	class Heading2 : public Html::Tag
	{
	public:
		Heading2 (Body const & body)
			: Html::Tag (body.GetStream (), "h2")
		{}
		Heading2 (std::ostream & stream)
			: Html::Tag (stream, "h2")
		{}
	};

	class Bold : public Html::Tag
	{
	public:
		Bold (Body const & body)
			: Html::Tag (body.GetStream (), "b")
		{}
		Bold (std::ostream & stream)
			: Html::Tag (stream, "b")
		{}
	};

	class Preformatted : public Html::Tag
	{
	public:
		Preformatted (std::ostream & stream)
			: Html::Tag (stream, "pre")
		{}
	};
}

#endif
