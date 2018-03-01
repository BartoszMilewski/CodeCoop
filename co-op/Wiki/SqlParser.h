#if !defined (SQLPARSER_H)
#define SQLPARSER_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------
#include "Sql.h"
#include "WikiLexer.h"
class QueryString;

namespace Sql
{
	enum Token 
	{
		tokError,
		tokEnd,
		tokString,
		// List
		tokComma,
		tokLParen,
		tokRParen,
		// Relational
		tokNotEq,
		tokEq,
		tokLess,
		tokLessOrEq,
		tokMore,
		tokMoreOrEq,
		tokContains,
		tokLike,
		tokIn,
		// Logical
		tokOr,
		tokAnd,
		tokNot,
		// Keywords
		tokSelect,
		tokInsert,
		tokInto,
		tokFrom,
		tokWhere,
		tokOrderBy,
		tokDesc
	};

	class ExprLexer
	{
	public:
		ExprLexer (WikiLexer & lex, QueryString const * fields, Sql::TuplesMap & tuples)
			: _lex (lex), _fields (fields), _tuples (tuples)
		{
			Analyze ();
		}
		void Accept ()
		{
			Analyze ();
		}
		Token GetToken () const { return _token; }
		std::string const & GetString () const { return _string; }
	private:
		void Analyze ();
	private:
		WikiLexer &	_lex;
		QueryString const * _fields;
		Sql::TuplesMap & _tuples;

		Token		_token;
		std::string	_string;
	};

	class CmdParser
	{
	public:
		CmdParser (WikiLexer & lex, QueryString const * fields, Sql::TuplesMap & tuples);
		~CmdParser ();
		Sql::Command::Type GetType () const 
		{ 
			return (_cmd.get () == 0)? Sql::Command::Invalid : _cmd->GetType (); 
		}
		bool IsError () const { return _isError; }
		// Must be called only once!
		std::unique_ptr<Sql::Command> ReleaseCommand () { return std::move(_cmd); }
		ExprLexer & GetLexer () { return _lex; }
	private:
		std::unique_ptr<Sql::Node> Predicate ();
		void Initializers (NocaseMap<std::string> & tuples);
		void Fields (std::vector<std::string> & fields, char const * terminator);
		std::string TableName (bool & isSystemTable);
	private:
		ExprLexer	_lex;
		bool		_isError;
		std::unique_ptr<Sql::Command> _cmd;
	};

	class ExprParser
	{
	public:
		ExprParser (ExprLexer & lex, bool & isError)
			: _lex (lex), _isError (isError)
		{}
		std::unique_ptr<Sql::Node> Expression ();
	private:
		std::unique_ptr<Sql::Node> Term ();
		std::unique_ptr<Sql::Node> Factor ();
		std::unique_ptr<Sql::Node> Comparison ();
		void Fields (NocaseSet & values);
	private:
		ExprLexer &		_lex;
		bool &			_isError;
	};
}

#endif
