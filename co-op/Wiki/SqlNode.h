#if !defined (SQLNODE_H)
#define SQLNODE_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------
#include <File/File.h>
#include <File/Path.h>
#include <StringOp.h>
#include <auto_vector.h>

namespace Sql
{
	typedef NocaseMap<std::string> TuplesMap;
	typedef std::vector<std::pair<std::string, std::string> > TuplesVector;

	class Node
	{
	public:
		virtual ~Node () {}
		virtual void GatherFields (NocaseSet & fields) const = 0;
		virtual bool Match (TuplesMap const & tuples) const = 0;
		virtual bool IsCount (unsigned & count) const { return false; }
		virtual void Dump (std::ostream & out, int indent) const = 0;
		void Indent (std::ostream & out, int indent) const
		{
			for (int i = 0; i < indent; ++i)
				out << " ";
		}
	};

	class MultiNode: public Node
	{
	public:
		MultiNode (std::unique_ptr<Node> child)
		{
			_children.push_back (std::move(child));
		}
		void AddChild (std::unique_ptr<Node> child)
		{
			if (child.get () != 0)
				_children.push_back (std::move(child));
		}
		void GatherFields (NocaseSet & fields) const
		{
			for (auto_vector<Node>::const_iterator it = _children.begin (); it != _children.end (); ++it)
			{
				(*it)->GatherFields (fields);
			}
		}
		void Dump (std::ostream & out, int indent) const
		{
			for (auto_vector<Node>::const_iterator it = _children.begin (); it != _children.end (); ++it)
			{
				(*it)->Dump (out, indent + 4);
				out << std::endl;
			}
		}
	protected:
		auto_vector<Node> _children;
	};

	class OrNode: public MultiNode
	{
	public:
		OrNode (std::unique_ptr<Node> child)
			: MultiNode (std::move(child))
		{}
		bool Match (TuplesMap const & tuples) const
		{
			for (auto_vector<Node>::const_iterator it = _children.begin (); it != _children.end (); ++it)
			{
				if ((*it)->Match (tuples))
					return true;
			}
			return false;
		}
		void Dump (std::ostream & out, int indent) const
		{
			Indent (out, indent);
			out << "OR" << std::endl;
			MultiNode::Dump (out, indent);
		}
	};

	class AndNode: public MultiNode
	{
	public:
		AndNode (std::unique_ptr<Node> child)
			: MultiNode (std::move(child))
		{}
		bool Match (TuplesMap const & tuples) const
		{
			for (auto_vector<Node>::const_iterator it = _children.begin (); it != _children.end (); ++it)
			{
				if (!(*it)->Match (tuples))
					return false;
			}
			return true;
		}
		bool IsCount (unsigned & count) const
		{
			for (auto_vector<Node>::const_iterator it = _children.begin (); it != _children.end (); ++it)
			{
				if ((*it)->IsCount (count))
					return true;
			}
			return false;
		}
		void Dump (std::ostream & out, int indent) const
		{
			Indent (out, indent);
			out << "AND" << std::endl;
			MultiNode::Dump (out, indent);
		}
	};

	class NotNode: public Node
	{
	public:
		NotNode (std::unique_ptr<Node> child)
			: _child (std::move(child))
		{}
		void GatherFields (NocaseSet & fields) const
		{
			_child->GatherFields (fields);
		}
		bool Match (TuplesMap const & tuples) const
		{
			return !_child->Match (tuples);
		}
		void Dump (std::ostream & out, int indent) const
		{
			Indent (out, indent);
			out << "NOT" << std::endl;
			_child->Dump (out, indent + 4);
		}
	private:
		std::unique_ptr<Node> _child;
	};

	class CountNode: public Node
	{
	public:
		CountNode (std::string const & value, int offset = 0)
		{
			if (!StrToUnsigned (value.c_str (), _value))
				_value = 0xffffffff;
			_value += offset;
		}
		void GatherFields (NocaseSet & fields) const
		{}
		bool Match  (TuplesMap const & tuples) const
		{
			return true;
		}
		bool IsCount (unsigned & count) const
		{
			count = _value;
			return true;
		}
		void Dump (std::ostream & out, int indent) const
		{
			Indent (out, indent);
			out << "COUNT = " << _value << std::endl;
		}
	private:
		unsigned long _value;
	};

	class CmpNode: public Node
	{
	public:
		CmpNode (std::string const & prop, std::string const & value)
			: _prop (prop), _value (value)
		{}
		void GatherFields (NocaseSet & fields) const
		{
			fields.insert (_prop);
		}
	protected:
		std::string _prop;
		std::string _value;
	};

	class EqNode: public CmpNode
	{
	public:
		EqNode (std::string const & prop, std::string const & value)
			: CmpNode (prop, value)
		{}
		bool Match (TuplesMap const & tuples) const
		{
			TuplesMap::const_iterator it = tuples.find (_prop);
			if (it == tuples.end ())
				return _value.empty ();
			return IsNocaseEqual (it->second, _value);
		}
		void Dump (std::ostream & out, int indent) const
		{
			Indent (out, indent);
			out << _prop << "=" << _value << std::endl;
		}
	};

	class NotEqNode: public CmpNode
	{
	public:
		NotEqNode (std::string const & prop, std::string const & value)
			: CmpNode (prop, value)
		{}
		bool Match (TuplesMap const & tuples) const
		{
			TuplesMap::const_iterator it = tuples.find (_prop);
			if (it == tuples.end ())
				return !_value.empty ();
			return !IsNocaseEqual (it->second, _value);
		}
		void Dump (std::ostream & out, int indent) const
		{
			Indent (out, indent);
			out << _prop << "<>" << _value << std::endl;
		}
	};

	class LessNode: public CmpNode
	{
	public:
		LessNode (std::string const & prop, std::string const & value)
			: CmpNode (prop, value)
		{}
		bool Match (TuplesMap const & tuples) const
		{
			TuplesMap::const_iterator it = tuples.find (_prop);
			if (it == tuples.end ())
				return !_value.empty ();
			return IsNocaseLess (it->second, _value);
		}
		void Dump (std::ostream & out, int indent) const
		{
			Indent (out, indent);
			out << _prop << "<" << _value << std::endl;
		}
	};

	class LessOrEqNode: public CmpNode
	{
	public:
		LessOrEqNode (std::string const & prop, std::string const & value)
			: CmpNode (prop, value)
		{}
		bool Match (TuplesMap const & tuples) const
		{
			TuplesMap::const_iterator it = tuples.find (_prop);
			if (it == tuples.end ())
				return true;
			return IsNocaseLess (it->second, _value) || IsNocaseEqual (it->second, _value);
		}
		void Dump (std::ostream & out, int indent) const
		{
			Indent (out, indent);
			out << _prop << "<=" << _value << std::endl;
		}
	};

	class MoreNode: public CmpNode
	{
	public:
		MoreNode (std::string const & prop, std::string const & value)
			: CmpNode (prop, value)
		{}
		bool Match (TuplesMap const & tuples) const
		{
			TuplesMap::const_iterator it = tuples.find (_prop);
			return it != tuples.end () 
				&& (!IsNocaseLess (it->second, _value) && !IsNocaseEqual (it->second, _value));
		}
		void Dump (std::ostream & out, int indent) const
		{
			Indent (out, indent);
			out << _prop << ">" << _value << std::endl;
		}
	};

	class MoreOrEqNode: public CmpNode
	{
	public:
		MoreOrEqNode (std::string const & prop, std::string const & value)
			: CmpNode (prop, value)
		{}
		bool Match (TuplesMap const & tuples) const
		{
			TuplesMap::const_iterator it = tuples.find (_prop);
			if (it == tuples.end ())
				return _value.empty ();
			return !IsNocaseLess (it->second, _value);
		}
		void Dump (std::ostream & out, int indent) const
		{
			Indent (out, indent);
			out << _prop << ">=" << _value << std::endl;
		}
	};

	class ContainsNode: public CmpNode
	{
	public:
		ContainsNode (std::string const & prop, std::string const & value)
			: CmpNode (prop, TrimmedString (value))
		{}
		bool Match (TuplesMap const & tuples) const;
		void Dump (std::ostream & out, int indent) const
		{
			Indent (out, indent);
			out << _prop << "CONTAINS" << _value << std::endl;
		}
	};

	class LikeNode: public CmpNode
	{
	public:
		LikeNode (std::string const & prop, std::string const & value)
			: CmpNode (prop, value)
		{}
		bool Match (TuplesMap const & tuples) const;
		void Dump (std::ostream & out, int indent) const
		{
			Indent (out, indent);
			out << _prop << "LIKE" << _value << std::endl;
		}
	private:
		bool MatchWildcard (std::string const & str, unsigned curStr, unsigned curPat) const;
	};

	class InNode: public Node
	{
	public:
		InNode (std::string const & prop, NocaseSet const & values)
			: _prop (prop), _values (values)
		{}
		bool Match (TuplesMap const & tuples) const;
		void GatherFields (NocaseSet & fields) const
		{
			fields.insert (_prop);
		}
		void Dump (std::ostream & out, int indent) const;
	private:
		std::string _prop;
		NocaseSet	_values;
	};
}
#endif
