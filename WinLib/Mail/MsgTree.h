#if !defined (MSGTREE_H)
#define MSGTREE_H
// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------
#include <File/Path.h>
#include <auto_vector.h>

class Socket;

// Email message single header
// Syntax: name: value; attribute
class Header
{
public:
	Header (std::string const & name, std::string const & value, std::string const & attribute)
		: _name (name),
		  _value (value),
		  _attribute (attribute)
	{}

	std::string const & GetName () const { return _name; }
	std::string const & GetValue () const { return _value; }
	std::string const & GetAttribute () const { return _attribute; }
private:
	std::string		_name;
	std::string		_value;
	std::string		_attribute;
};

class MessagePart
{
public:
	virtual ~MessagePart () {}

	void SetHeaders (std::vector<Header> const & headers) { _headers = headers; }
	Header const * FindHeader (std::string const & name) const;

	virtual bool IsMultiPart () const { return false; }
	virtual void AgreeOnBoundary (std::string & boundary) const {}
	virtual void Send (Socket & socket) const = 0;
protected:
	std::vector<Header>	_headers;
};

class MultipartMixedPart : public MessagePart
{
public:
	void AddPart (std::unique_ptr<MessagePart> part)
	{
		_parts.push_back (std::move(part));
	}
	bool IsMultiPart () const { return true; }
	void Send (Socket & socket) const;
private:
	auto_vector<MessagePart>	_parts;
};

class PlainTextPart : public MessagePart
{
public:
	PlainTextPart (std::string const & text)
		: _text (text)
	{}
	void AgreeOnBoundary (std::string & boundary) const;
	void Send (Socket & socket) const;
private:
	std::string const _text;
};

class ApplicationOctetStreamPart : public MessagePart
{
public:
	ApplicationOctetStreamPart (FilePath const & attPath);
	void Send (Socket & socket) const;
private:
	FilePath const	_attPath;
	std::string		_filename;
};

#endif
