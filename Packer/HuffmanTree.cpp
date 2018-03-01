//-----------------------------------------------------
//  (c) Reliable Software 2001 - 2004
//-----------------------------------------------------

#include "precompiled.h"
#include "HuffmanTree.h"
#include "BitCode.h"
#include "BitStream.h"

#include <queue>

HuffmanTree::HuffmanTree (std::vector<unsigned int> const & frequency)
	: _root (0)
{
	GreaterFrequency freqCmp;
	// The priority queue implements queue from which elements are read according
	// to their priority. In our case the elements are read in the order of lower
	// frequencies. The top queue elemenent has the lowest frequency among all
	// elements in the queue. If more then one element with lowest frequency exists,
	// which element comes next is undefined.
	std::priority_queue<Node const *, std::vector<Node const *>, GreaterFrequency> queue (freqCmp);
	// Create leaf nodes and initialize priority queue
	for (unsigned int i = 0; i < frequency.size (); ++i)
	{
		if (frequency [i] != 0)
		{
			// Create leaf node for alphabet symbol that actually appeared
			// in the compresed text. Frequency vector contains frequencies
			// for all alphabet symbols -- in our case numbers from range <0; size - 1>
			std::unique_ptr<Node> tmp (new Leaf (frequency [i], i));
			_nodes.push_back (std::move(tmp));
			queue.push (_nodes.back ());
		}
	}
	unsigned int leafNodeCount = _nodes.size ();
	if (leafNodeCount == 0)
	{
		// Empty Huffman tree
		std::unique_ptr<Node> tmp (new BinNode (0, 0, 0));
		_nodes.push_back(std::move(tmp));
		_root = _nodes.back ();
		return;
	}
	else if (leafNodeCount == 1)
	{
		// Just one leaf in the tree
		std::unique_ptr<Node> tmp (new BinNode (0, _nodes.back (), 0));
		_nodes.push_back(std::move(tmp));
		_root = _nodes.back ();
		return;
	}
	// Build Huffman tree -- bottom-up
	for (unsigned int k = 0; k < leafNodeCount - 1; ++k)
	{
		Assert (queue.size () >= 2);
		// Remove from the priority queue two nodes with the smallest frequencies
		Node const * left = queue.top ();
		int leftFreq = left->GetFrequency ();
		queue.pop ();
		Assert (!queue.empty ());
		Node const * right = queue.top ();
		int rightFreq = right->GetFrequency ();
		queue.pop ();
		// Create internal node and add it to the priority queue
		int internalFreq = leftFreq + rightFreq;
		std::unique_ptr<Node> tmp (new BinNode (internalFreq, left, right));
		_nodes.push_back(std::move(tmp));
		queue.push (_nodes.back ());
	}
	Assert (queue.size () == 1);
	_root = queue.top ();
}

class CodeBinLenCollector : public NodeVisitor
{
public:
	CodeBinLenCollector (std::vector<unsigned int> & codeBinLen)
		: _codeBinLen (codeBinLen),
		  _treeDepth (0)
	{}

	void BeforeVisitingChildren ()
	{
		_treeDepth++;
	}
	void AfterVisitingChildren ()
	{
		_treeDepth--;
	}
	void VisitNode (unsigned int symbol)
	{
		Assert (symbol < _codeBinLen.size ());
		_codeBinLen [symbol] = _treeDepth;
	}

private:
	std::vector<unsigned int> &	_codeBinLen;
	long						_treeDepth;
};

void HuffmanTree::CalcCodeBitLen (std::vector<unsigned int> & codeBitLen) const
{
	Assert (_root != 0);
	CodeBinLenCollector lenCollector (codeBitLen);
	_root->Visit (lenCollector);
}

// Modified Huffman tree assigns prefix codes having the following properties:
//		1. All codes of a given bit length have lexicographically
//		   consecutive values, in the same order as the symbols
//		   they represent.
//		2. Shorter codes lexicographically precede longer codes.

CodeTree::CodeTree (CodeBitLengths const & huffmanCode)
{
	std::vector<Leaf const *> leaves;
	// Create leaf nodes
	unsigned int symbol = 0;
	for (CodeBitLengths::Iterator iter = huffmanCode.begin ();
		 iter != huffmanCode.end ();
		 ++iter, ++symbol)
	{
		unsigned int codeBitLen = *iter;
		if (codeBitLen != 0)
		{
			// Create leaf node for alphabet symbol with non zero code bit lenght
			std::unique_ptr<Leaf> tmp (new Leaf (symbol, codeBitLen));
			leaves.push_back (tmp.get ());
			_nodes.push_back (std::move(tmp));
		}
	}
	if (_nodes.size () == 0)
	{
		// Empty Huffman tree
		std::unique_ptr<Node> tmp (new BinNode ());
		_nodes.push_back(std::move(tmp));
		_root = _nodes.back ();
		return;
	}
	else if (_nodes.size () == 1)
	{
		// Just one leaf in the tree
		std::unique_ptr<Node> tmp (new BinNode ());
		tmp->MakeLeftChild (_nodes.back ());
		_nodes.push_back(std::move(tmp));
		_root = _nodes.back ();
		return;
	}
	// Sort leaf nodes according to their code bit lengths
	LessCodeBitLen bitLenCmp;
	std::stable_sort (leaves.begin (), leaves.end (), bitLenCmp);
	BuildTree (leaves);
}

CodeTree::CodeTree (CodeRange const * const codeRange, unsigned int count)
{
	std::vector<Leaf const *> leaves;
	// Create leaf nodes
	for (unsigned int i = 0; i < count; ++i)
	{
		CodeRange const & range = codeRange [i];
		unsigned int symbol = range.GetFirstSymbol ();
		unsigned int lastSymbol = range.GetLastSymbol ();
		unsigned int codeBitLen = range.GetBitLength ();
		for ( ; symbol <= lastSymbol; ++symbol)
		{
			// Create leaf node for alphabet symbol
			std::unique_ptr<Leaf> tmp (new Leaf (symbol, codeBitLen));
			leaves.push_back (tmp.get ());
			_nodes.push_back (std::move(tmp));
		}
	}
	Assert (_nodes.size () > 1);
	BuildTree (leaves);
}

class CodeCollector : public NodeVisitor
{
public:
	CodeCollector (std::vector<BitCode> & codeTable)
		: _codeTable (codeTable),
		  _treeDepth (0),
		  _code (0)
	{}

	void BeforeVisitingChildren ()
	{
		_treeDepth++;
		_code <<= 1;
	}
	void BetweenVisitingChildren ()
	{
		_code |= 1;
	}
	void AfterVisitingChildren ()
	{
		_treeDepth--;
		_code >>= 1;
	}
	void VisitNode (unsigned int symbol)
	{
		Assert (symbol < _codeTable.size ());
		_codeTable [symbol] = BitCode (_code, _treeDepth);
	}

private:
	std::vector<BitCode> &	_codeTable;
	unsigned int			_treeDepth;
	unsigned int			_code;
};

void CodeTree::CalcCodes (std::vector<BitCode> & codeTable) const
{
	Assert (_root != 0);
	CodeCollector codeCollector (codeTable);
	_root->Visit (codeCollector);
}

unsigned int CodeTree::DecodeSymbol (InputBitStream & input) const
{
	Assert (_root != 0);
	Node const * node = _root;
	for ( ; ; )
	{
		bool bit = input.NextBit () == 1;
		if (bit)
			node = static_cast<BinNode const *>(node)->RightChild ();
		else
			node = static_cast<BinNode const *>(node)->LeftChild ();
		Leaf const * leaf = dynamic_cast<Leaf const *>(node);
		if (leaf != 0)
			return leaf->GetSymbol ();
	}
}

void CodeTree::BuildTree (std::vector<Leaf const *> const & leaves)
{
	//-------------------------------------
	// Build modified Huffman tree top-down
	//-------------------------------------
	// Childless binary nodes from previous level (to be given children at current level)
	std::vector<Node *> previousLevelNodes;

	// Start with the root at depth 0
	std::unique_ptr<Node> root (new BinNode ());
	_nodes.push_back (std::move(root));
	_root = _nodes.back ();
	previousLevelNodes.push_back (_nodes.back ());

	std::vector<Leaf const *>::const_iterator curLeaf = leaves.begin ();
	// Each leaf node should be placed at the tree level (depth) 
	// equal to its Huffman code bit-length
	for (unsigned int treeDepth = 1; !previousLevelNodes.empty (); ++treeDepth)
	{
		// Iterate over all childless parent nodes and give them children
		std::vector<Node *>::iterator parentNodeIter = previousLevelNodes.begin ();
		bool nextChildLeft = true;
		// First use all leaf nodes that have code bit-lenght equal to the current tree depth
		while (parentNodeIter != previousLevelNodes.end () &&
			   (*curLeaf)->GetHuffmanCodeBitLen () == treeDepth)
		{
			// as long as there are childless parent nodes, 
			// we MUST have leaf nodes
			Assert (curLeaf != leaves.end ());
			if (nextChildLeft)
			{
				(*parentNodeIter)->MakeLeftChild (*curLeaf);
				nextChildLeft = false;
			}
			else
			{
				(*parentNodeIter)->MakeRightChild (*curLeaf);
				++parentNodeIter;
				nextChildLeft = true;
			}
			++curLeaf;
		}
		// Fill the rest of parent nodes with new childless binary nodes
		std::vector<Node *> newBinaryNodes;
		while (parentNodeIter != previousLevelNodes.end ())
		{
			// Create new internal node at current tree depth
			std::unique_ptr<Node> tmp (new BinNode ());
			_nodes.push_back(std::move(tmp));
			newBinaryNodes.push_back (_nodes.back ());
			if (nextChildLeft)
			{
				(*parentNodeIter)->MakeLeftChild (_nodes.back ());
				nextChildLeft = false;
			}
			else
			{
				(*parentNodeIter)->MakeRightChild (_nodes.back ());
				++parentNodeIter;
				nextChildLeft = true;
			}
		}
		// childless binary nodes from current level
		// become previous level nodes to be filled with children
		previousLevelNodes.swap (newBinaryNodes);
	}
	if (curLeaf != leaves.end ())
	{
		Win::ClearError ();
		throw Win::Exception ("Corrupted compressed file -- cannot build Huffman tree.");
	}
}

#if !defined (NDEBUG)
void CodeBitLengths::Dump (char const * name) const
{
	//Msg info;
	//info << "Code bit lengths for " << name << "; size = " << _bitLen.size () << "\n";
	//DebugOut (info.c_str ());
	for (unsigned int i = 0; i < _bitLen.size (); ++i)
	{
		//Msg info1;
		//info1 << i << ": " << _bitLen [i] << "\n";
		//DebugOut (info1.c_str ());
	}
}

bool CodeBitLengths::IsEqual (CodeBitLengths const & codeBitLengths) const
{
	if (_bitLen.size () != codeBitLengths._bitLen.size ())
		return false;
	for (unsigned int i = 0; i < _bitLen.size (); ++i)
	{
		unsigned int len = _bitLen [i];
		unsigned int len1 = codeBitLengths._bitLen [i];
		if (len != len1)
			return false;
	}
	return true;
}
#endif
