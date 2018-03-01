#if !defined (SERIALUNVECTOR_H)
#define SERIALUNVECTOR_H
// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------

#include <unmovable_vector.h>
#include <File/FileIo.h>

template <class T>
class SerialUnVector : public unmovable_vector<T>
{
public:
	class Output
	{
	public:
		Output (unmovable_vector<T> & output)
			: _output (output)
		{}
		void Put (T const & elem) { _output.push_back (elem); }
	private:
		unmovable_vector<T>	& _output;
	};
public:
	void Save (FileIo & outFile) const
	{
		for (BlocksIter curr = _blocks.begin (); curr != _blocks.end (); ++curr)
		{
			outFile.Write ((*curr)->begin (), (*curr)->size () * sizeof (T));
		}
	}
};

#endif
