#if !defined (HUFFMANTREE_H)
#define HUFFMANTREE_H
//-----------------------------------------------------
//  HuffmanTree.h
//  (c) Reliable Software 2001 -- 2002
//-----------------------------------------------------

#include <auto_vector.h>

class InputBitStream;

class NodeVisitor
{
public:
	virtual ~NodeVisitor () {}

	virtual void VisitNode (unsigned int dataIndex) = 0;
	virtual	void BeforeVisitingChildren () {}
	virtual void BetweenVisitingChildren () {}
	virtual void AfterVisitingChildren () {}
};

class HuffmanTree
{
public:
	HuffmanTree (std::vector<unsigned int> const & frequency);

	void CalcCodeBitLen (std::vector<unsigned int> & codeBitLen) const;

private:
	class Node
	{
	public:
		virtual ~Node () {}

		virtual void Visit (NodeVisitor & visitor) const = 0;

		unsigned int GetFrequency () const { return _frequency; }

	protected:
		Node (unsigned int frequency)
			: _frequency (frequency)
		{}

	private:
		unsigned int	_frequency;
	};

	class BinNode : public Node
	{
	public:
		BinNode (unsigned int frequency, Node const * left, Node const * right)
			: Node (frequency),
			  _left (left),
			  _right (right)
		{}

		void Visit (NodeVisitor & visitor) const
		{
			visitor.BeforeVisitingChildren ();
			if (_left != 0)		// This can happen in the degenerated tree
				_left->Visit (visitor);
			visitor.BetweenVisitingChildren ();
			if (_right != 0)	// This can happen in the degenerated tree
				_right->Visit (visitor);
			visitor.AfterVisitingChildren ();
		}

	private:
		Node const *	_left;
		Node const *	_right;
	};

	class Leaf : public Node
	{
	public:
		Leaf (unsigned int frequency, unsigned int symbol)
			: Node (frequency),
			  _symbol (symbol)
		{}

		void Visit (NodeVisitor & visitor) const
		{
			visitor.VisitNode (_symbol);
		}

	private:
		unsigned int	_symbol;	// Alphabet symbol represented by this leaf
	};

	class GreaterFrequency : public std::binary_function<Node const *, Node const *, bool>
	{
	public:
		bool operator () (Node const * node1, Node const * node2) const
		{
			return node1->GetFrequency () > node2->GetFrequency ();
		}
	};

private:
	auto_vector<Node>	_nodes;
	Node const *		_root;
};

class CodeRange
{
public:
	CodeRange (unsigned int firstSymbol, unsigned int lastSymbol, unsigned int bitLength, unsigned int firstCode)
		: _firstSymbol (firstSymbol),
		  _lastSymbol (lastSymbol),
		  _bitLength (bitLength),
		  _firstCode (firstCode)
	{}

	unsigned int GetFirstSymbol () const { return _firstSymbol; }
	unsigned int GetLastSymbol () const { return _lastSymbol; }
	unsigned int GetBitLength () const { return _bitLength; }
	unsigned int GetFirstCode () const { return _firstCode; }

private:
	unsigned int	_firstSymbol;
	unsigned int	_lastSymbol;
	unsigned int	_bitLength;
	unsigned int	_firstCode;
};

class CodeBitLengths
{
	friend class CodeTree;

public:
	CodeBitLengths ()
	{}
	CodeBitLengths (CodeBitLengths const & codeBitLengths)
		: _bitLen (codeBitLengths._bitLen)
	{}
	CodeBitLengths (HuffmanTree const & tree, unsigned int symbolCount)
		: _bitLen (symbolCount)
	{
		tree.CalcCodeBitLen (_bitLen);
	}
	CodeBitLengths (unsigned int symbolCount)
		: _bitLen (symbolCount)
	{}

	unsigned int size () const { return _bitLen.size (); }
	unsigned int & operator [] (unsigned int index) { return _bitLen [index]; }
	unsigned int at (unsigned int index) const { return _bitLen.at (index); }
	void push_back (unsigned int codeBitLen) { _bitLen.push_back (codeBitLen); }

	typedef std::vector<unsigned int>::const_iterator Iterator;

	Iterator begin () const { return _bitLen.begin (); }
	Iterator end () const { return _bitLen.end (); }

#if !defined (NDEBUG)
	void Dump (char const * name) const;
	bool IsEqual (CodeBitLengths const & codeBitLengths) const;
#endif

private:
	std::vector<unsigned int>	_bitLen;
};

class BitCode;

class CodeTree
{
public:
	CodeTree (CodeBitLengths const & huffmanCodeBitLen);
	CodeTree (CodeRange const * const codeRange, unsigned int count);

	void CalcCodes (std::vector<BitCode> & code) const;
	unsigned int DecodeSymbol (InputBitStream & input) const;

private:
	class Node
	{
	public:
		virtual ~Node () {}

		virtual void Visit (NodeVisitor & visitor) const = 0;
		virtual void MakeLeftChild (Node const * child) { }
		virtual void MakeRightChild (Node const * child) { }
	};

	class BinNode : public Node
	{
	public:
		BinNode ()
			: _left (0),
			  _right (0)
		{}

		Node const * LeftChild () const { return _left; }
		Node const * RightChild () const { return _right; }

		void MakeLeftChild (Node const * child) { _left = child; }
		void MakeRightChild (Node const * child) { _right = child; }

		void Visit (NodeVisitor & visitor) const
		{
			visitor.BeforeVisitingChildren ();
			if (_left != 0)		// This can happen in the degenerated tree
				_left->Visit (visitor);
			visitor.BetweenVisitingChildren ();
			if (_right != 0)	// This can happen in the degenerated tree
				_right->Visit (visitor);
			visitor.AfterVisitingChildren ();
		}

	private:
		Node const *	_left;
		Node const *	_right;
	};

	class Leaf : public Node
	{
	public:
		Leaf (unsigned int symbol, unsigned int huffmanCodeBitLen)
			: _symbol (symbol),
			  _huffmanCodeBitLen (huffmanCodeBitLen)
		{}

		unsigned int GetHuffmanCodeBitLen () const { return _huffmanCodeBitLen; }
		unsigned int GetSymbol () const { return _symbol; }

		void Visit (NodeVisitor & visitor) const
		{
			visitor.VisitNode (_symbol);
		}

	private:
		unsigned int	_symbol;	// Alphabet symbol represented by this leaf
		unsigned int	_huffmanCodeBitLen;
	};

	class LessCodeBitLen : public std::binary_function<Leaf const *, Leaf const *, bool>
	{
	public:
		bool operator () (Leaf const * node1, Leaf const * node2) const
		{
			return node1->GetHuffmanCodeBitLen () < node2->GetHuffmanCodeBitLen ();
		}
	};

	void BuildTree (std::vector<Leaf const *> const & leaves);

private:
	auto_vector<Node>	_nodes;
	Node const *		_root;
};

#endif
