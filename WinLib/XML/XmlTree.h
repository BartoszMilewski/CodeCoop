#if !defined (XMLTREE_H)
#define XMLTREE_H
// ----------------------------------
// (c) Reliable Software, 2003 - 2005
// ----------------------------------

#include "Sink.h"

#include <auto_vector.h>
#include <stack>

namespace XML
{
	class Exception : public Win::InternalException
	{
	public:
		Exception (char const * msg, char const * objName = 0)
			: Win::InternalException (msg, objName)
		{}
	};

	class Attribute
	{
	public:
		Attribute (std::string const & name, std::string const & value)
		: _name (name), 
		  _value (value)
		{}
		Attribute (std::string const & name)
			: _name (name)
		{}

		std::string const & GetName  () const { return _name;  }
		std::string const & GetValue () const { return _value; }

		void SetTransformValue (std::string const & val);
		std::string GetTransformValue () const;
	private:
		std::string _name;
		std::string _value;
	};
	
	class Node
	{
	public:
		explicit Node (std::string const & name, bool closed = false)
			: _name (name), _closed (closed)
		{}
		void SetName (std::string const & name) { _name = name; }
		Node * AddChild (std::unique_ptr<Node> child);
		Node * AddChild (std::string const & name);
		Node * AddEmptyChild (std::string const & name);
		bool RemoveAttribute (std::string const & attrName);
		void AddAttribute (std::string const & name, std::string const & value);
		void AddAttribute (std::string const & name, int value);
		void AddTransformAttribute (std::string const & name, std::string const & value);
		void AddText (std::string const & text);
		void AddTransformText (std::string const & text);
		void WriteOpeningTag (std::ostream & out, unsigned indent) const;
		void WriteClosingTag (std::ostream & out, unsigned indent) const;
		void Write (std::ostream & out, unsigned indent = 0) const;

		std::string const & GetName () const { return _name; }
		std::string GetClosingTag () const;
		Node const * FindFirstChildNamed (std::string const & name) const throw ();
		Node const * GetFirstChildNamed (std::string const & name) const;
		Attribute const * FindAttribute (std::string const & name) const throw ();
		std::string const & GetAttribValue (std::string const & name) const;
		std::string GetTransformAttribValue (std::string const & name) const;
		std::string FindAttribValue (std::string const & attribName) const;
		std::string FindChildAttribValue (std::string const & childName, std::string const & attribName) const;
		// Return child with matching name and atrribute value
		Node const * FindChildByAttrib (std::string const & childName, 
										std::string const & attrName, 
										std::string const & attrValue) const;
		Node * FindEditChildByAttrib (std::string const & childName, 
										std::string const & attrName, 
										std::string const & attrValue);

		bool HasChildren () const { return _children.size () != 0; }
		typedef auto_vector<Node>::const_iterator ConstChildIter;
		typedef auto_vector<Node>::iterator ChildIter;
		ChildIter FirstChild () { return _children.begin (); }
		ChildIter LastChild  () { return _children.end  ();  }   
		ConstChildIter FirstChild () const { return _children.begin (); }
		ConstChildIter LastChild  () const { return _children.end  ();  }   
		typedef auto_vector<Attribute>::const_iterator ConstAttribIter;
		ConstAttribIter FirstAttrib () const { return _attributes.begin (); }
		ConstAttribIter LastAttrib  () const { return _attributes.end  ();  } 
		std::unique_ptr<Node> PopChild () 
		{
			Assert (_children.size () != 0);
			return _children.pop_back ();
		}
	private:
		std::string			   _name;
		auto_vector<Attribute> _attributes;
		auto_vector<Node>      _children;
		bool _closed; // of the form <name attrib="value"/>
	};

	class Tree
	{
		friend class TreeMaker;
	public:
		Tree ()
			: _top ("Top")
		{}
		Node * SetRoot (std::unique_ptr<Node> root)
		{
			Assert (!_top.HasChildren ());
			return _top.AddChild (std::move(root));
		}
		Node * SetRoot (std::string const & name);
		Node const * GetRoot () const 
		{
			if (_top.HasChildren ())
				return *_top.FirstChild ();
			else
				return 0;
		}
		Node * GetRootEdit () 
		{
			if (_top.HasChildren ())
				return *_top.FirstChild ();
			else
				return 0;
		}
		void swap (Tree & srcTree);
		static void WriteHeader (std::ostream & out);
		void Write (std::ostream & out) const;
	private:
		std::unique_ptr<Node> PopRoot ()
		{
			Assert (_top.HasChildren ());
			return _top.PopChild ();
		}
	private:
		Node _top;
	};

	class TreeMaker : public Sink
	{
	public:
		TreeMaker (Tree & tree)
			: _curr (&tree._top)
		{}
		// Sink interface
		void OnStartTag (std::string const & tagName);
		void OnEndTag ();
		void OnEndTagEmpty ();
		void OnAttribute (std::string const & attrib, std::string const & value);
		void OnText (std::string const & text);
		void OnError (XML::Error const & error);
private:
		Node *			      _curr;
		std::stack<Node*>	  _parents;
	};
}

#endif
