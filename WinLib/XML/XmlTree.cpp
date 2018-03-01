// ----------------------------------
// (c) Reliable Software, 2003 - 2005
// ----------------------------------

#include "WinLibBase.h"
#include "XmlTree.h"

#include <StringOp.h>

namespace XML
{
	char SpecialLookup [256] = 
	{
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	};

	class IsSpecial // for faster execution
	{
	public:
		bool operator () (unsigned char c) const // inlined
		{ 
			return SpecialLookup [c] != 0; 
		}
	};

	std::string const Quot = "&quot;";
	std::string const Amp  = "&amp;";
	std::string const Apos = "&apos;";
	std::string const Lt   = "&lt;";
	std::string const Gt   = "&gt;";

	std::string TransformSpecial (char c)
	{
		if (IsAscii (c))
		{
			switch (c)
			{
			case 34:
				return Quot;
			case 38:
				return Amp;
			case 39:
				return Apos;
			case 60:
				return Lt;
			case 62:
				return Gt;
			default:
				Assert (!"Character is not special!");
				return Quot;
			};
		}
		else
		{
			std::string encodedUpperAscii ("&#x");
			encodedUpperAscii += ToHexStr (c);
			
			encodedUpperAscii += ';';
			return encodedUpperAscii;
		}
	}

	// ---------
	// Attribute
	// ---------

	void Attribute::SetTransformValue (std::string const & value)
	{
		std::string::const_iterator current = 
			std::find_if (value.begin (), value.end (), IsSpecial ());
		if (current == value.end ())
		{
			_value = value;
			return;
		}

		std::string::const_iterator firstNotAdded = value.begin ();
		do 
		{
			Assert (current != value.end ());
			_value.append (firstNotAdded, current);
			_value.append (XML::TransformSpecial (*current));
			++current;
			firstNotAdded = current;
			current = std::find_if (current, value.end (), IsSpecial ());
		} while (current != value.end ());
		_value.append (firstNotAdded, value.end ());
	}

	std::string Attribute::GetTransformValue () const
	{
		std::string::size_type current = _value.find ('&');
		if (current == std::string::npos)
			return _value;

		std::string::const_iterator firstNotAdded = _value.begin ();
		std::string result;
		do
		{
			Assert (current != std::string::npos);

			result.append (firstNotAdded, _value.begin () +  current);
			// find semicolon
			std::string::size_type semicolonPos = _value.find (';', current + 1);
			if (semicolonPos == std::string::npos
				|| semicolonPos - current < 3 
				|| semicolonPos - current > 5)
				throw XML::Exception ("Illegal ampersand character in XML document.");

			std::string special = _value.substr (current, semicolonPos - current + 1);
			if (special == Quot)
			{
				result += '"';
			}
			else if (special == Amp)
			{
				result += '&';
			}
			else if (special == Apos)
			{
				result += '\'';
			}
			else if (special == Lt)
			{
				result += '<';
			}
			else if (special == Gt)
			{
				result += '>';
			}
			else
			{
				// check for hex upper ascii: e.g. "&#xff;" .
				if (special.length () == 6
					&& _value [current + 1] == '#'
					&& _value [current + 2] == 'x'
					&& std::isxdigit (_value [current + 3])
					&& std::isxdigit (_value [current + 4])
					&& _value [current + 5] == ';')
				{
					unsigned long ch;
					if (!HexStrToUnsigned (_value.substr (current + 3, 2).c_str (), ch))
						throw XML::Exception ("Illegal ampersand character in XML document.");
					result += static_cast<char> (ch);
				}
				else
					throw XML::Exception ("Illegal ampersand character in XML document.");
			}
			current += special.length ();
			firstNotAdded = _value.begin () + current;
			current = _value.find ('&', current);
		} while (current != std::string::npos);
		result.append (firstNotAdded, _value.end ());
		return result;		
	}

	// ----
	// Tree
	// ----

	void Tree::WriteHeader (std::ostream & out)
	{
		out << "<?xml version=\"1.0\" ?>" << std::endl;
	}
	void Tree::Write (std::ostream & out) const
	{
		WriteHeader (out);
		GetRoot ()->Write (out);
	}
	Node * Tree::SetRoot (std::string const & name)
	{
		Assert (!_top.HasChildren ());
		std::unique_ptr<Node> root (new Node (name));
		return _top.AddChild (std::move(root));
	}

	void Tree::swap (Tree & srcTree)
	{
		if (srcTree.GetRoot () != 0)
		{
			std::unique_ptr<Node> srcRoot = srcTree.PopRoot ();
			if (GetRoot () != 0)
			{
				std::unique_ptr<Node> myRoot = _top.PopChild ();
				srcTree.SetRoot (std::move(myRoot));
			}
			SetRoot (std::move(srcRoot));
		}
	}

	class IsEqualName
	{
	public:
		explicit IsEqualName (std::string const & name)
			: _searchedName (name)
		{}
		bool operator () (Node const * node)
		{
			// XML is case sensitive
			return _searchedName == node->GetName ();
		}
		bool operator () (Attribute const * attrib)
		{
			// XML is case sensitive
			return _searchedName == attrib->GetName ();
		}
	private:
		std::string const & _searchedName;
	};

	//-----
	// Node
	//-----

	Node * Node::AddChild (std::unique_ptr<Node> child)
	{
		if (_closed)
			throw XML::Exception ("Cannot add child to a closed XML node");
		_children.push_back (std::move(child));
		return _children.back ();
	}

	Node * Node::AddChild (std::string const & name)
	{
		std::unique_ptr<Node> child (new Node (name));
		return AddChild (std::move(child));
	}

	Node * Node::AddEmptyChild (std::string const & name)
	{
		std::unique_ptr<Node> child (new Node (name, true));
		return AddChild (std::move(child));
	}

	bool Node::RemoveAttribute (std::string const & attrName)
	{
		for (unsigned i = 0; i < _attributes.size (); ++ i)
		{
			if (_attributes [i]->GetName () == attrName)
			{
				_attributes.erase (i);
				return true;
			}
		}
		return false;
	}

	void Node::AddAttribute (std::string const & name, std::string const & value)
	{
		_attributes.push_back (std::unique_ptr<Attribute> (new Attribute (name, value)));
	}

	void Node::AddAttribute (std::string const & name, int value)
	{
		AddAttribute (name, ToString (value));
	}

	void Node::AddTransformAttribute (std::string const & name, std::string const & value)
	{
		std::unique_ptr<Attribute> attrib (new Attribute (name));
		attrib->SetTransformValue (value);
		_attributes.push_back (std::move(attrib));
	}

	void Node::AddText (std::string const & text)
	{
		if (_closed)
			throw XML::Exception ("Cannot add text to a closed XML node");
		// text as a nameless child with Text attribute
		std::unique_ptr<Node> child (new Node (std::string ()));
		Node * newChild = AddChild (std::move(child));
		newChild->AddAttribute ("Text", text);
	}

	void Node::AddTransformText (std::string const & text)
	{
		if (_closed)
			throw XML::Exception ("Cannot add text to a closed XML node");
		// text as a nameless child with Text attribute
		std::unique_ptr<Node> child (new Node (std::string ()));
		Node * newChild = AddChild (std::move(child));
		newChild->AddTransformAttribute ("Text", text);
	}

	void Node::WriteOpeningTag (std::ostream & out, unsigned indent) const
	{
		out << Indentation (indent) << "<" << _name;
		for (ConstAttribIter it = FirstAttrib (); it != LastAttrib (); ++it)
		{
			out << " " << (*it)->GetName () << "=\"" << (*it)->GetValue () << "\"";
		}
		if (_closed)
		{
			out << "/>" << std::endl;
		}
		else
		{
			out << ">" << std::endl;
		}
	}

	void Node::WriteClosingTag (std::ostream & out, unsigned indent) const
	{
		if (!_closed)
		{
			out << Indentation (indent) << "</" << _name << '>' << std::endl;
		}
	}

	void Node::Write (std::ostream & out, unsigned indent) const
	{
		WriteOpeningTag (out, indent);
		if (!_closed)
		{
			for (ConstChildIter it = FirstChild (); it != LastChild (); ++it)
			{
				XML::Node * child = *it;
				if (child->GetName ().empty ()) // text
				{
					out << Indentation (indent) << child->GetAttribValue ("Text") << std::endl;
				}
				else
				{
					child->Write (out, indent + 2);
				}
			}
		}
		WriteClosingTag (out, indent);
	}

	Node const * Node::FindFirstChildNamed (std::string const & name) const throw ()
	{
		ConstChildIter it = 
				std::find_if (_children.begin (),
						      _children.end (),
						      IsEqualName (name));
		if (it != _children.end ())
		{
			return *it;
		}
		return 0;
	}

	Node const * Node::GetFirstChildNamed (std::string const & name) const
	{
		Node const * node = FindFirstChildNamed (name);
		if (node == 0)
			throw XML::Exception ("Node not found");

		return node;
	}

	Attribute const * Node::FindAttribute (std::string const & name) const throw ()
	{
		ConstAttribIter it = 
				std::find_if (_attributes.begin (),
						      _attributes.end (),
						      IsEqualName (name));
		if (it != _attributes.end ())
		{
			return *it;
		}
		return 0;
	}

	std::string const & Node::GetAttribValue (std::string const & name) const
	{
		Attribute const * attrib = FindAttribute (name);
		if (attrib == 0)
		{
			throw XML::Exception ("Attribute not found");
		}
		return attrib->GetValue ();
	}

	std::string Node::GetTransformAttribValue (std::string const & name) const
	{
		Attribute const * attrib = FindAttribute (name);
		if (attrib == 0)
		{
			throw XML::Exception ("Attribute not found");
		}
		return attrib->GetTransformValue ();
	}

	std::string Node::FindAttribValue (std::string const & attribName) const
	{
		XML::Attribute const * attrib = FindAttribute (attribName);
		if (attrib != 0)
			return attrib->GetValue ();

		return std::string ();
	}

	std::string Node::FindChildAttribValue (std::string const & childName, std::string const & attribName) const
	{
		XML::Node const * child = FindFirstChildNamed (childName);
		if (child != 0)
		{
			return child->FindAttribValue (attribName);
		}
		return std::string ();
	}



	Node const * Node::FindChildByAttrib (	std::string const & childName, 
											std::string const & attrName, 
											std::string const & attrValue) const
	{
		XML::Node::ConstChildIter it = FirstChild ();
		XML::Node::ConstChildIter last = LastChild ();
		while (it != last)
		{
			XML::Node const * child = *it;
			if (child->GetName () == childName && child->FindAttribValue (attrName) == attrValue)
				return child;
			++it;
		}
		return 0;
	}

	Node * Node::FindEditChildByAttrib (	std::string const & childName, 
											std::string const & attrName, 
											std::string const & attrValue)
	{
		XML::Node::ChildIter it = FirstChild ();
		XML::Node::ChildIter last = LastChild ();
		while (it != last)
		{
			XML::Node * child = *it;
			if (child->GetName () == childName && child->FindAttribValue (attrName) == attrValue)
				return child;
			++it;
		}
		return 0;
	}

	//----------
	// TreeMaker
	//----------
	
	void TreeMaker::OnStartTag (std::string const & tagName)
	{
		std::unique_ptr<Node> child (new Node (tagName));
		Node * newChild = _curr->AddChild (std::move(child));
		_parents.push (_curr);
		_curr = newChild;
	}

	void TreeMaker::OnEndTag ()
	{
		Assert (!_parents.empty ()); // Parser must not allow such call
		_curr = _parents.top ();
		_parents.pop ();
	}

	void TreeMaker::OnEndTagEmpty ()
	{
		OnEndTag ();
	}

	void TreeMaker::OnAttribute (std::string const & attrib, std::string const & value)
	{
		Assert (_curr != 0); // Parser must not allow such call
		_curr->AddAttribute (attrib, value); 
	}

	void TreeMaker::OnText (std::string const & text)
	{
		Assert (_curr != 0); // Parser must not allow such call
		std::unique_ptr<Node> child (new Node (std::string ()));
		Node * newChild = _curr->AddChild (std::move(child));
		newChild->AddAttribute ("Text", text);
	}
	void TreeMaker::OnError (XML::Error const & error)
	{
		throw XML::Exception ("Syntax error in XML document");
	}
}

namespace UnitTest
{
	void XmlTree (std::ostream & out)
	{
		XML::Tree tree;
		XML::Node * root = tree.SetRoot ("UnitTest");
		std::unique_ptr<XML::Node> child1 (new XML::Node ("child"));
		child1->AddAttribute ("num", "1");
		child1->AddTransformAttribute ("level", "1");
		child1->AddTransformAttribute ("upperExamples", 
				"some upper chars: ¡, ², Ã or Ôå and finally ö.");
		child1->AddTransformText ("This text contains all 5 special characters:\n"
			"ampersand: &, quotation mark: \", apostrophe: ', less than: < "
			"and greater than char: >.");
		root->AddChild (std::move(child1));

		root->AddText ("This is my next child");

		XML::Node * node = root->AddChild ("child");
		node->AddAttribute ("num", 2);
		node->AddAttribute ("level", 1);
		XML::Node * grandChild = node->AddEmptyChild ("Grandchild");
		grandChild->AddAttribute ("age", 2);
		tree.Write (out);

		XML::Node const * firstChild = *tree.GetRoot ()->FirstChild ();
		out << "\n\nTransform back upper-ascii characters: " 
			<< firstChild->GetTransformAttribValue ("upperExamples") << std::endl;
		out << "\nTransform back the text: " 
			<< (*firstChild->FirstChild ())->GetTransformAttribValue ("Text") << std::endl;
	}
}
