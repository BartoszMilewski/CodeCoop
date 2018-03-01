#if !defined DECOMPRESSOR_H
#define DECOMPRESSOR_H
//-------------------------------------
// (c) Reliable Software 1999-2003
// ------------------------------------
#include <File/File.h>
class OutNamedBlock;
class OutBlockVector;

class Decompressor
{
public: 
	bool Decompress (unsigned char const * buf, File::Size size, OutBlockVector & outBlocks);

private:
	class Buffer
	{
	public:
		Buffer (OutNamedBlock & block);

		void Write (unsigned char byte);
		void Copy (unsigned int distance, unsigned int count);
		void Flush ();

	private:
		OutNamedBlock &				_block;
		std::vector<unsigned char>	_buf;
		unsigned int				_bufWritePos;
	};
};

#endif
