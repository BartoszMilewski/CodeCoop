//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------
#include "precompiled.h"
#include "SqlParser.h"
#include "WikiConverter.h" // WikiLexer
#include "WikiDef.h"
#include "Registry.h"

void Sql::ExprLexer::Analyze ()
{
	_lex.EatWhiteToEnd ();
	char c = _lex.Get ();
	switch (c)
	{
	case '\n':
		_lex.Accept ();
		_token = tokEnd;
		break;
	case '\0':
		_token = tokEnd;
		break;
	case ',':
		_token = tokComma;
		_lex.Accept ();
		break;
	case '<':
		_lex.Accept ();
		if (_lex.Get () == '>')
		{
			_token = tokNotEq;
			_lex.Accept ();
		}
		else if (_lex.Get () == '=')
		{
			_token = tokLessOrEq;
			_lex.Accept ();
		}
		else
			_token = tokLess;
		break;
	case '>':
		_lex.Accept ();
		if (_lex.Get () == '=')
		{
			_token = tokMoreOrEq;
			_lex.Accept ();
		}
		else
			_token = tokMore;
		break;
	case '=':
		_lex.Accept ();
		if (_lex.Get () == '=') // alternative syntax
			_lex.Accept ();
		_token = tokEq;
		break;
	case '(':
		_lex.Accept ();
		_token = tokLParen;
		break;
	case ')':
		_lex.Accept ();
		_token = tokRParen;
		break;
	case '"':
		_token = tokString;
		_string = _lex.GetQuotedString ();
		break;
	case '[':
		{
			_lex.Accept (); // [
			c = _lex.Get ();
			if (c != '?' && c != '!' && c != '@')
			{
				_token = tokError;
				break;
			}
			_lex.Accept (); // ? or ! or @
			std::string name (_lex.GetString ("]"));
			if (name.size () == 0 || _lex.Get () != ']')
			{
				_token = tokError;
				break;
			}
			_lex.Accept (); // ]
			if (c == '?')
			{
				if (_fields)
					_string = _fields->GetField (name);
				else
					_string = name;
			}
			else if (c == '!')
			{
				if (_tuples.find (name) == _tuples.end ())
					_string = "\"\"";
				else
					_string = _tuples [name];
			}
			else if (c == '@')
			{
				Registry::UserWikiPrefs prefs;
				_string = prefs.GetPropertyValue (name);
				if (_string.size () == 0)
					_string = "not in registry";
			}
			_token = tokString;
			break;
		}
	default:
		{
			_string = _lex.GetString (",<>=[]()\"");
			if (_string == "OR")
				_token = tokOr;
			else if (_string == "AND")
				_token = tokAnd;
			else if (_string == "NOT")
				_token = tokNot;
			else if (_string == "FROM")
				_token = tokFrom;
			else if (_string == "SELECT")
				_token = tokSelect;
			else if (_string == "INSERT")
				_token = tokInsert;
			else if (_string == "INTO")
				_token = tokInto;
			else if (_string == "WHERE")
				_token = tokWhere;
			else if (_string == "ORDER")
			{
				_lex.EatWhite ();
				if (_lex.IsMatch ("BY", true))
				{
					_lex.Accept (2);
					_token = tokOrderBy;
				}
				else
					_token = tokString;
			}
			else if (_string == "DESC")
				_token = tokDesc;
			else if (_string == "CONTAINS")
				_token = tokContains;
			else if (_string == "LIKE")
				_token = tokLike;
			else if (_string == "IN")
				_token = tokIn;
			else
				_token = tokString;
		}
	}
}

Sql::CmdParser::CmdParser (WikiLexer & lex, QueryString const * fields, Sql::TuplesMap & tuples)
	: _lex (lex, fields, tuples), _isError (false)
{
	Token tok = _lex.GetToken ();
	if (tok == tokInsert)
	{
		_lex.Accept ();
		tok = _lex.GetToken ();
		if (tok != tokInto)
		{
			_isError = true;
			return;
		}
		_lex.Accept ();
		tok = _lex.GetToken ();
		if (tok != tokString)
		{
			_isError = true;
			return;
		}
		bool isSystemTable;
		std::string table = TableName (isSystemTable);
		std::unique_ptr<Sql::InsertCommand> cmd (new Sql::InsertCommand (table, isSystemTable));
		Initializers (cmd->GetTuples ());
		_cmd = std::move(cmd);
	}
	else if (tok == tokFrom)
	{
		// Special form of SELECT command
		_lex.Accept ();
		tok = _lex.GetToken ();
		if (tok != tokString)
		{
			_isError = true;
			return;
		}
		
		bool isSystemTable;
		std::string table = TableName (isSystemTable);

		tok = _lex.GetToken ();
		std::unique_ptr<Node> predicate;
		if (tok == tokWhere)
		{
			_lex.Accept ();
			predicate = Predicate ();
		}
		std::unique_ptr<Sql::FromCommand> cmd (new Sql::FromCommand (table, isSystemTable));
		cmd->AddPredicate (std::move(predicate));
		_cmd = std::move(cmd);
	}
	else if (tok == tokSelect)
	{
		_lex.Accept ();
		std::unique_ptr<Sql::SelectCommand> cmd (new Sql::SelectCommand);
		Fields (cmd->GetSelectFields (), "FROM");
		tok = _lex.GetToken ();
		if (tok != tokFrom)
		{
			_isError = true;
			return;
		}
		_lex.Accept ();
		tok = _lex.GetToken ();
		if (tok != tokString)
		{
			_isError = true;
			return;
		}
		bool isSystemTable;
		std::string table = TableName (isSystemTable);
		cmd->SetTableName (table, isSystemTable);
		tok = _lex.GetToken ();
		std::unique_ptr<Node> predicate;
		if (tok == tokWhere)
		{
			_lex.Accept ();
			predicate = Predicate ();
			tok = _lex.GetToken ();
		}
		cmd->AddPredicate (std::move(predicate));
		if (tok == tokOrderBy)
		{
			_lex.Accept ();
			std::vector<std::string> sortFields;
			Fields (sortFields, "DESC");
			tok = _lex.GetToken ();
			bool isDescending = false;
			if (tok == tokDesc)
			{
				_lex.Accept ();
				isDescending = true;
			}
			if (sortFields.size () != 0)
				cmd->AddSort (sortFields.front (), isDescending);
		}
		_cmd = std::move(cmd);
	}
	else
		_isError = true;

	if (_lex.GetToken () != tokEnd)
		_isError = true;
}

Sql::CmdParser::~CmdParser ()
{}

std::string Sql::CmdParser::TableName (bool & isSystemTable)
{
	std::string table = _lex.GetString ();
	_lex.Accept ();
	if (table.size () > Wiki::SYSTEM_PREFIX_SIZE 
		&& table.substr (0, Wiki::SYSTEM_PREFIX_SIZE) == Wiki::SYSTEM_PREFIX)
	{
		isSystemTable = true;
	}
	else
		isSystemTable = false;

	if (isSystemTable)
		return table.substr (Wiki::SYSTEM_PREFIX_SIZE);
	else
		return table;
}

std::unique_ptr<Sql::Node> Sql::CmdParser::Predicate ()
{
	Sql::ExprParser parser (GetLexer (), _isError);
	return parser.Expression ();
}

void Sql::Canonicalize (std::string & name)
{
	if (name == "FILENAME")
		name = FileNamePseudoProp;
	else if (name == "DELETE")
		name = DeletePseudoProp;
}
bool Sql::IsPseudo (std::string const & name)
{
	return name == FileNamePseudoProp
		|| name == DeletePseudoProp;
}

std::string Sql::DeCanonicalize (std::string const & name)
{
	if (name == FileNamePseudoProp)
		return "FILENAME";
	else if (name == DeletePseudoProp)
		return "DELETE";
	Assert (!"SQL::Decanonicalize called with non-pseudo property");
	return std::string ();
}

void Sql::CmdParser::Initializers (NocaseMap<std::string> & tuples)
{
	if (_lex.GetToken () != tokLParen)
		return;
	_lex.Accept ();
	Token tok = _lex.GetToken ();

	while (tok == tokString)
	{
		std::string name (_lex.GetString ());
		_lex.Accept ();
		Canonicalize (name);
		tok = _lex.GetToken ();
		if (tok != tokEq)
		{
			_isError = true;
			return;
		}
		_lex.Accept (); // '='
		tok = _lex.GetToken ();
		if (tok != tokString)
		{
			_isError = true;
			return;
		}
		std::string value (_lex.GetString ());

		tuples [name] = value;
		_lex.Accept ();
		tok = _lex.GetToken ();
		if (tok != tokComma)
			break;
		_lex.Accept (); // ','
		tok = _lex.GetToken ();
	}
	if (tok != tokRParen)
	{
		_isError = true;
	}
	_lex.Accept (); // ')'
}

void Sql::ExprParser::Fields (NocaseSet & values)
{
	Token tok = _lex.GetToken ();
	while (tok == tokString)
	{
		std::string field = _lex.GetString ();
		_lex.Accept ();

		Canonicalize (field);

		if (!field.empty ())
			values.insert (field);

		tok = _lex.GetToken ();
		if (tok == tokComma)
		{
			_lex.Accept (); // ','
			tok = _lex.GetToken ();
		}
	}
}

void Sql::CmdParser::Fields (std::vector<std::string> & fields, char const * terminator)
{
	Token tok = _lex.GetToken ();
	while (tok == tokString)
	{
		std::string field = _lex.GetString ();
		_lex.Accept ();
		if (field == terminator)
			break;
		Canonicalize (field);

		if (!field.empty ())
			fields.push_back (field);

		tok = _lex.GetToken ();
		if (tok == tokComma)
		{
			_lex.Accept (); // ','
			tok = _lex.GetToken ();
		}
	}
}

//---------------
// Sql ExprParser
//---------------

std::unique_ptr<Sql::Node> Sql::ExprParser::Expression ()
{
	std::unique_ptr<Sql::Node> term (Term ());
	Token tok = _lex.GetToken ();
	if (tok == tokOr)
	{
		std::unique_ptr<OrNode> orNode (new Sql::OrNode (std::move(term)));
		do
		{
			_lex.Accept ();
			orNode->AddChild (Term ());
			tok = _lex.GetToken ();
		} while (tok == tokOr);
		term = std::move(orNode);
	}
	return term;
}

std::unique_ptr<Sql::Node> Sql::ExprParser::Term ()
{
	std::unique_ptr<Sql::Node> factor (Factor ());
	Token tok = _lex.GetToken ();
	if (tok == tokAnd)
	{
		std::unique_ptr<AndNode> andNode (new Sql::AndNode (std::move(factor)));
		do
		{
			_lex.Accept ();
			andNode->AddChild (Factor());
			tok = _lex.GetToken ();
		} while (tok == tokAnd);
		factor = std::move(andNode);
	}
	return factor;
}

std::unique_ptr<Sql::Node> Sql::ExprParser::Factor ()
{
	std::unique_ptr<Sql::Node> node;
	Token tok = _lex.GetToken ();
	if (tok == tokLParen)
	{
		_lex.Accept (); // '('
		node = Expression ();
		if (_lex.GetToken () == tokRParen)
			_lex.Accept ();
		else
		{
			_isError = true;
			node.reset ();
		}
	}
	else if (tok == tokNot)
	{
		_lex.Accept (); // NOT
		node.reset (new Sql::NotNode (Factor ()));
	}
	else if (tok == tokString)
	{
		node = Comparison ();
	}
	else
		_isError = true;
	return node;
}

std::unique_ptr<Sql::Node> Sql::ExprParser::Comparison ()
{
	std::string prop = _lex.GetString ();
	_lex.Accept ();
	Canonicalize (prop);
	std::unique_ptr<Sql::Node> node;
	Token tok = _lex.GetToken ();
	if (tok == tokEq)
	{
		_lex.Accept (); // '='
		tok = _lex.GetToken ();
		if (tok == tokString)
		{
			std::string value = _lex.GetString ();
			_lex.Accept ();
			if (value.size () > 3 && value.substr (0, 3) == "NOT")
			{
				TrimmedString realValue (value.substr (3));
				node.reset (new NotEqNode (prop, realValue));
			}
			else if (prop == "COUNT")
				node.reset (new CountNode (value));
			else
				node.reset (new EqNode (prop, value));
		}
		else
			_isError = true;
	}
	else if (tok == tokNotEq)
	{
		_lex.Accept (); // '<>'
		tok = _lex.GetToken ();
		if (tok == tokString)
		{
			std::string value = _lex.GetString ();
			_lex.Accept ();
			node.reset (new NotEqNode (prop, value));
		}
		else
			_isError = true;
	}
	else if (tok == tokLess)
	{
		_lex.Accept (); // '<'
		tok = _lex.GetToken ();
		if (tok == tokString)
		{
			std::string value = _lex.GetString ();
			_lex.Accept ();
			if (prop == "COUNT")
				node.reset (new CountNode (value, -1));
			else
				node.reset (new LessNode (prop, value));
		}
		else
			_isError = true;
	}
	else if (tok == tokLessOrEq)
	{
		_lex.Accept (); // '<='
		tok = _lex.GetToken ();
		if (tok == tokString)
		{
			std::string value = _lex.GetString ();
			_lex.Accept ();
			if (prop == "COUNT")
				node.reset (new CountNode (value));
			else
				node.reset (new LessOrEqNode (prop, value));
		}
		else
			_isError = true;
	}
	else if (tok == tokMore)
	{
		_lex.Accept (); // '>'
		tok = _lex.GetToken ();
		if (tok == tokString)
		{
			std::string value = _lex.GetString ();
			_lex.Accept ();
			node.reset (new MoreNode (prop, value));
		}
		else
			_isError = true;
	}
	else if (tok == tokMoreOrEq)
	{
		_lex.Accept (); // '>='
		tok = _lex.GetToken ();
		if (tok == tokString)
		{
			std::string value = _lex.GetString ();
			_lex.Accept ();
			node.reset (new MoreOrEqNode (prop, value));
		}
		else
			_isError = true;
	}
	else if (tok == tokContains)
	{
		_lex.Accept (); // 'CONTAINS'
		tok = _lex.GetToken ();
		if (tok == tokString)
		{
			std::string value = _lex.GetString ();
			_lex.Accept ();
			node.reset (new ContainsNode (prop, value));
		}
		else
			_isError = true;
	}
	else if (tok == tokLike)
	{
		_lex.Accept (); // 'LIKE'
		tok = _lex.GetToken ();
		if (tok == tokString)
		{
			std::string value = _lex.GetString ();
			_lex.Accept ();
			node.reset (new LikeNode (prop, value));
		}
		else
			_isError = true;
	}
	else if (tok == tokIn)
	{
		_lex.Accept (); // 'IN'
		tok = _lex.GetToken ();
		if (tok == tokLParen)
		{
			_lex.Accept (); // '('
			NocaseSet values;
			Fields (values);
			if (_lex.GetToken () == tokRParen)
			{
				_lex.Accept (); // ')'
				node.reset (new InNode (prop, values));
			}
			else
				_isError = true;
		}
		else
			_isError = true;
	}
	else
	{
		_isError = true;
	}
	return node;
}

